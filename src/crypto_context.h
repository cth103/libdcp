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


/** @file  src/crypto_context.h
 *  @class CryptoContext class
 */


#ifndef LIBDCP_CRYPTO_CONTEXT_H
#define LIBDCP_CRYPTO_CONTEXT_H


#include "key.h"
#include "types.h"
#include "exceptions.h"
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_prng.h>
#include <boost/optional.hpp>


namespace dcp {


template <class T>
class CryptoContext
{
public:
	CryptoContext (boost::optional<Key> key, Standard standard)
		: _context (nullptr)
		, _hmac (nullptr)
	{
		if (!key) {
			return;
		}

		_context = new T ();
		if (ASDCP_FAILURE (_context->InitKey(key->value()))) {
			throw MiscError ("could not set up crypto context");
		}

		uint8_t cbc_buffer[ASDCP::CBC_BLOCK_SIZE];

		Kumu::FortunaRNG rng;
		if (ASDCP_FAILURE (_context->SetIVec (rng.FillRandom (cbc_buffer, ASDCP::CBC_BLOCK_SIZE)))) {
			throw MiscError ("could not set up CBC initialization vector");
		}

		_hmac = new ASDCP::HMACContext;

		ASDCP::LabelSet_t type;
		if (standard == Standard::INTEROP) {
			type = ASDCP::LS_MXF_INTEROP;
		} else {
			type = ASDCP::LS_MXF_SMPTE;
		}

		if (ASDCP_FAILURE (_hmac->InitKey(key->value(), type))) {
			throw MiscError ("could not set up HMAC context");
		}
	}

	~CryptoContext ()
	{
		delete _context;
		delete _hmac;
	}

	T* context () const {
		return _context;
	}

	ASDCP::HMACContext* hmac () const {
		return _hmac;
	}

private:
	T* _context;
	ASDCP::HMACContext* _hmac;
};


typedef CryptoContext<ASDCP::AESEncContext> EncryptionContext;
typedef CryptoContext<ASDCP::AESDecContext> DecryptionContext;


}


#endif
