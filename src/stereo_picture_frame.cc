/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#include "stereo_picture_frame.h"
#include "exceptions.h"
#include "util.h"
#include "rgb_xyz.h"
#include "colour_conversion.h"
#include "AS_DCP.h"
#include "KM_fileio.h"
#include "compose.hpp"
#include "j2k.h"
#include <openjpeg.h>

using std::string;
using boost::shared_ptr;
using namespace dcp;

/** Make a picture frame from a 3D (stereoscopic) asset.
 *  @param mxf_path Path to the asset's MXF file.
 *  @param n Frame within the asset, not taking EntryPoint into account.
 */
StereoPictureFrame::StereoPictureFrame (boost::filesystem::path path, int n)
{
	ASDCP::JP2K::MXFSReader reader;
	Kumu::Result_t r = reader.OpenRead (path.string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (FileError ("could not open MXF file for reading", path, r));
	}

	/* XXX: unfortunate guesswork on this buffer size */
	_buffer = new ASDCP::JP2K::SFrameBuffer (4 * Kumu::Megabyte);

	if (ASDCP_FAILURE (reader.ReadFrame (n, *_buffer))) {
		boost::throw_exception (DCPReadError (String::compose ("could not read video frame %1 of %2", n, path.string())));
	}
}

StereoPictureFrame::StereoPictureFrame ()
{
	_buffer = new ASDCP::JP2K::SFrameBuffer (4 * Kumu::Megabyte);
}

StereoPictureFrame::~StereoPictureFrame ()
{
	delete _buffer;
}

/** @param eye Eye to return (EYE_LEFT or EYE_RIGHT).
 *  @param reduce a factor by which to reduce the resolution
 *  of the image, expressed as a power of two (pass 0 for no
 *  reduction).
 */
shared_ptr<OpenJPEGImage>
StereoPictureFrame::xyz_image (Eye eye, int reduce) const
{
	switch (eye) {
	case LEFT:
		return decompress_j2k (const_cast<uint8_t*> (_buffer->Left.RoData()), _buffer->Left.Size(), reduce);
	case RIGHT:
		return decompress_j2k (const_cast<uint8_t*> (_buffer->Right.RoData()), _buffer->Right.Size(), reduce);
	}

	return shared_ptr<OpenJPEGImage> ();
}

uint8_t const *
StereoPictureFrame::left_j2k_data () const
{
	return _buffer->Left.RoData ();
}

uint8_t*
StereoPictureFrame::left_j2k_data ()
{
	return _buffer->Left.Data ();
}

int
StereoPictureFrame::left_j2k_size () const
{
	return _buffer->Left.Size ();
}

uint8_t const *
StereoPictureFrame::right_j2k_data () const
{
	return _buffer->Right.RoData ();
}

uint8_t*
StereoPictureFrame::right_j2k_data ()
{
	return _buffer->Right.Data ();
}

int
StereoPictureFrame::right_j2k_size () const
{
	return _buffer->Right.Size ();
}
