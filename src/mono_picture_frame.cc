/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

    This file is part of libdcp.

    libdcp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libdcp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdcp.  If not, see <http://www.gnu.org/licenses/>.

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/


/** @file  src/mono_picture_frame.cc
 *  @brief MonoPictureFrame class
 */


#include "colour_conversion.h"
#include "compose.hpp"
#include "crypto_context.h"
#include "exceptions.h"
#include "j2k_transcode.h"
#include "mono_picture_frame.h"
#include "rgb_xyz.h"
#include "util.h"
#include <asdcp/KM_fileio.h>
#include <asdcp/AS_DCP.h>


using std::make_shared;
using std::string;
using std::shared_ptr;
using boost::optional;
using namespace dcp;


MonoPictureFrame::MonoPictureFrame (boost::filesystem::path path)
{
	auto const size = boost::filesystem::file_size (path);
	_buffer.reset(new ASDCP::JP2K::FrameBuffer(size));
	auto f = fopen_boost (path, "rb");
	if (!f) {
		boost::throw_exception (FileError("could not open JPEG2000 file", path, errno));
	}

	size_t n = fread (data(), 1, size, f);
	fclose (f);

	if (n != size) {
		boost::throw_exception (FileError("could not read from JPEG2000 file", path, errno));
	}

	_buffer->Size (size);
}


/** Make a picture frame from a 2D (monoscopic) asset.
 *  @param reader Reader for the asset's MXF file.
 *  @param n Frame within the asset, not taking EntryPoint into account.
 *  @param c Context for decryption, or 0.
 */
MonoPictureFrame::MonoPictureFrame (ASDCP::JP2K::MXFReader* reader, int n, shared_ptr<DecryptionContext> c)
{
	/* XXX: unfortunate guesswork on this buffer size */
	_buffer = make_shared<ASDCP::JP2K::FrameBuffer>(4 * Kumu::Megabyte);

	auto const r = reader->ReadFrame (n, *_buffer, c->context(), c->hmac());

	if (ASDCP_FAILURE(r)) {
		boost::throw_exception (ReadError(String::compose ("could not read video frame %1 (%2)", n, static_cast<int>(r))));
	}
}


MonoPictureFrame::MonoPictureFrame (uint8_t const * data, int size)
{
	_buffer = make_shared<ASDCP::JP2K::FrameBuffer>(size);
	_buffer->Size (size);
	memcpy (_buffer->Data(), data, size);
}


uint8_t const *
MonoPictureFrame::data () const
{
	return _buffer->RoData ();
}


uint8_t *
MonoPictureFrame::data ()
{
	return _buffer->Data ();
}


int
MonoPictureFrame::size () const
{
	return _buffer->Size ();
}


shared_ptr<OpenJPEGImage>
MonoPictureFrame::xyz_image (int reduce) const
{
	return decompress_j2k (const_cast<uint8_t*>(_buffer->RoData()), _buffer->Size(), reduce);
}
