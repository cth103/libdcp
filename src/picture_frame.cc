/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <openjpeg.h>
#include "AS_DCP.h"
#include "KM_fileio.h"
#include "picture_frame.h"
#include "exceptions.h"
#include "argb_frame.h"
#include "lut.h"

using namespace std;
using namespace boost;
using namespace libdcp;

PictureFrame::PictureFrame (string mxf_path, int n)
{
	ASDCP::JP2K::MXFReader reader;
	if (ASDCP_FAILURE (reader.OpenRead (mxf_path.c_str()))) {
		throw FileError ("could not open MXF file for reading", mxf_path);
	}

	/* XXX: unfortunate guesswork on this buffer size */
	_buffer = new ASDCP::JP2K::FrameBuffer (4 * Kumu::Megabyte);

	if (ASDCP_FAILURE (reader.ReadFrame (n, *_buffer))) {
		throw DCPReadError ("could not read video frame");
	}
}

PictureFrame::~PictureFrame ()
{
	delete _buffer;
}

uint8_t const *
PictureFrame::data () const
{
	return _buffer->RoData();
}

int
PictureFrame::size () const
{
	return _buffer->Size ();
}

/** @return An ARGB representation of this frame.  This is ARGB in the
 *  Cairo sense, so that each pixel takes up 4 bytes; the first byte
 *  is blue, second green, third red and fourth alpha (always 255).
 */
shared_ptr<ARGBFrame>
PictureFrame::argb_frame () const
{
	/* JPEG2000 -> decompressed XYZ */
	
	opj_dinfo_t* decoder = opj_create_decompress (CODEC_J2K);
	opj_dparameters_t parameters;
	opj_set_default_decoder_parameters (&parameters);
	opj_setup_decoder (decoder, &parameters);
	opj_cio_t* cio = opj_cio_open ((opj_common_ptr) decoder, const_cast<unsigned char *> (data()), size());
	opj_image_t* xyz_frame = opj_decode (decoder, cio);
	if (!xyz_frame) {
		opj_destroy_decompress (decoder);
		opj_cio_close (cio);
		throw DCPReadError ("could not decode JPEG2000 codestream");
	}
	
	assert (xyz_frame->numcomps == 3);
	
	/* XYZ -> RGB */
	
	struct {
		double x, y, z;
	} s;
	
	struct {
		double r, g, b;
	} d;
	
	int* xyz_x = xyz_frame->comps[0].data;
	int* xyz_y = xyz_frame->comps[1].data;
	int* xyz_z = xyz_frame->comps[2].data;

	shared_ptr<ARGBFrame> argb_frame (new ARGBFrame (xyz_frame->x1, xyz_frame->y1));
	
	uint8_t* argb = argb_frame->data ();
	
	for (int y = 0; y < xyz_frame->y1; ++y) {
		uint8_t* argb_line = argb;
		for (int x = 0; x < xyz_frame->x1; ++x) {
			
			assert (*xyz_x >= 0 && *xyz_y >= 0 && *xyz_z >= 0 && *xyz_x < 4096 && *xyz_x < 4096 && *xyz_z < 4096);
			
			/* In gamma LUT */
			s.x = lut_in[*xyz_x++];
			s.y = lut_in[*xyz_y++];
			s.z = lut_in[*xyz_z++];
			
			/* DCI companding */
			s.x /= DCI_COEFFICIENT;
			s.y /= DCI_COEFFICIENT;
			s.z /= DCI_COEFFICIENT;
			
			/* XYZ to RGB */
			d.r = ((s.x * color_matrix[0][0]) + (s.y * color_matrix[0][1]) + (s.z * color_matrix[0][2]));
			d.g = ((s.x * color_matrix[1][0]) + (s.y * color_matrix[1][1]) + (s.z * color_matrix[1][2]));
			d.b = ((s.x * color_matrix[2][0]) + (s.y * color_matrix[2][1]) + (s.z * color_matrix[2][2]));
			
			d.r = min (d.r, 1.0);
			d.r = max (d.r, 0.0);
			
			d.g = min (d.g, 1.0);
			d.g = max (d.g, 0.0);
			
			d.b = min (d.b, 1.0);
			d.b = max (d.b, 0.0);
			
			/* Out gamma LUT */
			*argb_line++ = lut_out[(int) (d.b * COLOR_DEPTH)];
			*argb_line++ = lut_out[(int) (d.g * COLOR_DEPTH)];
			*argb_line++ = lut_out[(int) (d.r * COLOR_DEPTH)];
			*argb_line++ = 0xff;
		}
		
		argb += argb_frame->stride ();
	}
	
	opj_cio_close (cio);
	opj_image_destroy (xyz_frame);

	return argb_frame;
}
