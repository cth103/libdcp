/*
    Copyright (C) 2023 Carl Hetherington <cth@carlh.net>

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


#include "compose.hpp"
#include "mono_mpeg2_picture_frame.h"


using std::make_shared;
using std::shared_ptr;
using namespace dcp;



/** Make a picture frame from a 2D (monoscopic) asset.
 *  @param reader Reader for the asset's MXF file.
 *  @param n Frame within the asset, not taking EntryPoint into account.
 *  @param c Context for decryption, or 0.
 *  @param check_hmac true to check the HMAC and give an error if it is not as expected.
 */
MonoMPEG2PictureFrame::MonoMPEG2PictureFrame(ASDCP::MPEG2::MXFReader* reader, int n, shared_ptr<DecryptionContext> context, bool check_hmac)
{
	/* XXX: unfortunate guesswork on this buffer size */
	_buffer = make_shared<ASDCP::MPEG2::FrameBuffer>(4 * Kumu::Megabyte);

	auto const r = reader->ReadFrame(n, *_buffer, context->context(), check_hmac ? context->hmac() : nullptr);

	if (ASDCP_FAILURE(r)) {
		boost::throw_exception(ReadError(String::compose("could not read video frame %1 (%2)", n, static_cast<int>(r))));
	}
}


uint8_t const *
MonoMPEG2PictureFrame::data() const
{
	return _buffer->RoData();
}


uint8_t *
MonoMPEG2PictureFrame::data()
{
	return _buffer->Data();
}


int
MonoMPEG2PictureFrame::size() const
{
	return _buffer->Size();
}

