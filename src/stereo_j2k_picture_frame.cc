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


/** @file  src/stereo_picture_frame.cc
 *  @brief StereoPictureFrame class
 */


#include "colour_conversion.h"
#include "compose.hpp"
#include "crypto_context.h"
#include "exceptions.h"
#include "j2k_transcode.h"
#include "rgb_xyz.h"
#include "stereo_picture_frame.h"
#include "util.h"
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_fileio.h>


using std::string;
using std::shared_ptr;
using std::make_shared;
using namespace dcp;


StereoPictureFrame::Part::Part (shared_ptr<ASDCP::JP2K::SFrameBuffer> buffer, Eye eye)
	: _buffer (buffer)
	, _eye (eye)
{

}


ASDCP::JP2K::FrameBuffer &
StereoPictureFrame::Part::mono () const
{
	return _eye == Eye::LEFT ? _buffer->Left : _buffer->Right;
}


uint8_t const *
StereoPictureFrame::Part::data () const
{
	return mono().RoData();
}


uint8_t *
StereoPictureFrame::Part::data ()
{
	return mono().Data();
}


int
StereoPictureFrame::Part::size () const
{
	return mono().Size();
}


/** Make a picture frame from a 3D (stereoscopic) asset.
 *  @param reader Reader for the MXF file.
 *  @param n Frame within the asset, not taking EntryPoint into account.
 *  @param check_hmac true to check the HMAC and give an error if it is not as expected.
 */
StereoPictureFrame::StereoPictureFrame (ASDCP::JP2K::MXFSReader* reader, int n, shared_ptr<DecryptionContext> c, bool check_hmac)
{
	/* XXX: unfortunate guesswork on this buffer size */
	_buffer = make_shared<ASDCP::JP2K::SFrameBuffer>(4 * Kumu::Megabyte);

	if (ASDCP_FAILURE (reader->ReadFrame (n, *_buffer, c->context(), check_hmac ? c->hmac() : nullptr))) {
		boost::throw_exception (ReadError (String::compose ("could not read video frame %1 of %2", n)));
	}
}


StereoPictureFrame::StereoPictureFrame ()
{
	_buffer = make_shared<ASDCP::JP2K::SFrameBuffer>(4 * Kumu::Megabyte);
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
	case Eye::LEFT:
		return decompress_j2k (const_cast<uint8_t*>(_buffer->Left.RoData()), _buffer->Left.Size(), reduce);
	case Eye::RIGHT:
		return decompress_j2k (const_cast<uint8_t*>(_buffer->Right.RoData()), _buffer->Right.Size(), reduce);
	}

	return {};
}


shared_ptr<StereoPictureFrame::Part>
StereoPictureFrame::right () const
{
	return make_shared<Part>(_buffer, Eye::RIGHT);
}


shared_ptr<StereoPictureFrame::Part>
StereoPictureFrame::left () const
{
	return make_shared<Part>(_buffer, Eye::LEFT);
}


