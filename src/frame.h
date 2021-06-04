/*
    Copyright (C) 2016-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/frame.h
 *  @brief Frame class
 */


#ifndef LIBDCP_FRAME_H
#define LIBDCP_FRAME_H


#include "crypto_context.h"
#include "exceptions.h"
#include <asdcp/KM_fileio.h>
#include <asdcp/AS_DCP.h>


namespace dcp {


template <class R, class B>
class Frame
{
public:
	Frame (R* reader, int n, std::shared_ptr<const DecryptionContext> c, bool check_hmac)
	{
		/* XXX: unfortunate guesswork on this buffer size */
		_buffer = std::make_shared<B>(Kumu::Megabyte);

		if (ASDCP_FAILURE(reader->ReadFrame(n, *_buffer, c->context(), check_hmac ? c->hmac() : nullptr))) {
			boost::throw_exception (ReadError ("could not read frame"));
		}
	}

	Frame (Frame const&) = delete;
	Frame& operator= (Frame const&) = delete;

	uint8_t const * data () const
	{
		return _buffer->RoData ();
	}

	int size () const
	{
		return _buffer->Size ();
	}

private:
	std::shared_ptr<B> _buffer;
};


}


#endif
