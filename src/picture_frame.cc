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
#include "util.h"

using namespace std;
using namespace boost;
using namespace libdcp;

MonoPictureFrame::MonoPictureFrame (string mxf_path, int n)
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

MonoPictureFrame::~MonoPictureFrame ()
{
	delete _buffer;
}

#if 0
uint8_t const *
MonoPictureFrame::data () const
{
	return _buffer->RoData();
}

int
MonoPictureFrame::size () const
{
	return _buffer->Size ();
}
#endif

/** @return An ARGB representation of this frame.  This is ARGB in the
 *  Cairo sense, so that each pixel takes up 4 bytes; the first byte
 *  is blue, second green, third red and fourth alpha (always 255).
 */
shared_ptr<ARGBFrame>
MonoPictureFrame::argb_frame () const
{
	opj_image_t* xyz_frame = decompress_j2k (const_cast<uint8_t*> (_buffer->RoData()), _buffer->Size());
	assert (xyz_frame->numcomps == 3);
	shared_ptr<ARGBFrame> f = xyz_to_rgb (xyz_frame);
	opj_image_destroy (xyz_frame);
	return f;
}


StereoPictureFrame::StereoPictureFrame (string mxf_path, int n)
{
	ASDCP::JP2K::MXFSReader reader;
	if (ASDCP_FAILURE (reader.OpenRead (mxf_path.c_str()))) {
		throw FileError ("could not open MXF file for reading", mxf_path);
	}

	/* XXX: unfortunate guesswork on this buffer size */
	_buffer = new ASDCP::JP2K::SFrameBuffer (4 * Kumu::Megabyte);

	if (ASDCP_FAILURE (reader.ReadFrame (n, *_buffer))) {
		throw DCPReadError ("could not read video frame");
	}
}

StereoPictureFrame::~StereoPictureFrame ()
{
	delete _buffer;
}

shared_ptr<ARGBFrame>
StereoPictureFrame::argb_frame (Eye eye) const
{
	opj_image_t* xyz_frame = 0;
	switch (eye) {
	case LEFT:
		xyz_frame = decompress_j2k (const_cast<uint8_t*> (_buffer->Left.RoData()), _buffer->Left.Size());
		break;
	case RIGHT:
		xyz_frame = decompress_j2k (const_cast<uint8_t*> (_buffer->Right.RoData()), _buffer->Right.Size());
		break;
	}
	
	assert (xyz_frame->numcomps == 3);
	shared_ptr<ARGBFrame> f = xyz_to_rgb (xyz_frame);
	opj_image_destroy (xyz_frame);
	return f;
}
