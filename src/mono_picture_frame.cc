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

/** @file  src/mono_picture_frame.cc
 *  @brief MonoPictureFrame class.
 */

#include "mono_picture_frame.h"
#include "exceptions.h"
#include "util.h"
#include "rgb_xyz.h"
#include "colour_conversion.h"
#include "KM_fileio.h"
#include "AS_DCP.h"
#include <openjpeg.h>

#define DCI_GAMMA 2.6

using std::string;
using boost::shared_ptr;
using boost::optional;
using namespace dcp;

/** Make a picture frame from a JPEG2000 file.
 *  @param path Path to JPEG2000 file.
 */
MonoPictureFrame::MonoPictureFrame (boost::filesystem::path path)
{
	boost::uintmax_t const size = boost::filesystem::file_size (path);
	_buffer = new ASDCP::JP2K::FrameBuffer (size);
	FILE* f = fopen_boost (path, "r");
	if (!f) {
		boost::throw_exception (FileError ("could not open JPEG2000 file", path, errno));
	}

	fread (j2k_data(), 1, size, f);
	fclose (f);

	_buffer->Size (size);
}

/** Make a picture frame from a 2D (monoscopic) asset.
 *  @param path Path to the asset's MXF file.
 *  @param n Frame within the asset, not taking EntryPoint into account.
 *  @param c Context for decryption, or 0.
 */
MonoPictureFrame::MonoPictureFrame (boost::filesystem::path path, int n, ASDCP::AESDecContext* c)
{
	ASDCP::JP2K::MXFReader reader;
	Kumu::Result_t r = reader.OpenRead (path.string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (FileError ("could not open MXF file for reading", path, r));
	}

	/* XXX: unfortunate guesswork on this buffer size */
	_buffer = new ASDCP::JP2K::FrameBuffer (4 * Kumu::Megabyte);

	if (ASDCP_FAILURE (reader.ReadFrame (n, *_buffer, c))) {
		boost::throw_exception (DCPReadError ("could not read video frame"));
	}
}

MonoPictureFrame::MonoPictureFrame ()
{
	_buffer = new ASDCP::JP2K::FrameBuffer (4 * Kumu::Megabyte);
}

/** MonoPictureFrame destructor */
MonoPictureFrame::~MonoPictureFrame ()
{
	delete _buffer;
}

/** @return Pointer to JPEG2000 data */
uint8_t const *
MonoPictureFrame::j2k_data () const
{
	return _buffer->RoData ();
}

/** @return Pointer to JPEG2000 data */
uint8_t *
MonoPictureFrame::j2k_data ()
{
	return _buffer->Data ();
}

/** @return Size of JPEG2000 data in bytes */
int
MonoPictureFrame::j2k_size () const
{
	return _buffer->Size ();
}

/** @param reduce a factor by which to reduce the resolution
 *  of the image, expressed as a power of two (pass 0 for no
 *  reduction).
 */
shared_ptr<XYZImage>
MonoPictureFrame::xyz_image (int reduce) const
{
	return decompress_j2k (const_cast<uint8_t*> (_buffer->RoData()), _buffer->Size(), reduce);
}
