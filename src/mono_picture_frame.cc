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
#include "mono_picture_frame.h"
#include "exceptions.h"
#include "argb_frame.h"
#include "lut.h"
#include "util.h"
#include "gamma_lut.h"
#include "rgb_xyz.h"

#define DCI_GAMMA 2.6

using std::string;
using boost::shared_ptr;
using namespace libdcp;

/** Make a picture frame from a 2D (monoscopic) asset.
 *  @param mxf_path Path to the asset's MXF file.
 *  @param n Frame within the asset, not taking EntryPoint into account.
 */
MonoPictureFrame::MonoPictureFrame (boost::filesystem::path mxf_path, int n, ASDCP::AESDecContext* c)
{
	ASDCP::JP2K::MXFReader reader;
	Kumu::Result_t r = reader.OpenRead (mxf_path.string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (FileError ("could not open MXF file for reading", mxf_path, r));
	}

	/* XXX: unfortunate guesswork on this buffer size */
	_buffer = new ASDCP::JP2K::FrameBuffer (4 * Kumu::Megabyte);

	if (ASDCP_FAILURE (reader.ReadFrame (n, *_buffer, c))) {
		boost::throw_exception (DCPReadError ("could not read video frame"));
	}
}

MonoPictureFrame::~MonoPictureFrame ()
{
	delete _buffer;
}

uint8_t const *
MonoPictureFrame::j2k_data () const
{
	return _buffer->RoData ();
}

int
MonoPictureFrame::j2k_size () const
{
	return _buffer->Size ();
}

/** @param reduce a factor by which to reduce the resolution
 *  of the image, expressed as a power of two (pass 0 for no
 *  reduction).
 *
 *  @return An ARGB representation of this frame.  This is ARGB in the
 *  Cairo sense, so that each pixel takes up 4 bytes; the first byte
 *  is blue, second green, third red and fourth alpha (always 255).
 *
 */
shared_ptr<ARGBFrame>
MonoPictureFrame::argb_frame (int reduce, float srgb_gamma) const
{
	return xyz_to_rgb (
		decompress_j2k (const_cast<uint8_t*> (_buffer->RoData()), _buffer->Size(), reduce),
		GammaLUT::cache.get (12, DCI_GAMMA), GammaLUT::cache.get (12, 1 / srgb_gamma)
		);
}
