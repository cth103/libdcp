/*
    Copyright (C) 2013-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/key.h
 *  @brief Key class
 */


#ifndef LIBDCP_KEY_H
#define LIBDCP_KEY_H


#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <asdcp/AS_DCP.h>
LIBDCP_ENABLE_WARNINGS
#include <stdint.h>
#include <string>


namespace dcp {


/** @class Key
 *  @brief A key for decrypting/encrypting assets
 */
class Key
{
public:
	/** Create a new, random key */
	explicit Key (int length = ASDCP::KeyLen);

	/** Create a Key from a raw key value */
	explicit Key (uint8_t const *, int length = ASDCP::KeyLen);

	/** Create a Key from a hex key value */
	explicit Key (std::string);

	Key (Key const &);
	Key& operator= (Key const &);

	~Key ();

	/** @return Raw key value */
	uint8_t const * value () const {
		return _value;
	}

	int length () const {
		return _length;
	}

	/** @return Key value as a hexadecimal string */
	std::string hex () const;

private:
	/** Raw key value */
	uint8_t* _value = nullptr;
	int _length = 0;
};


extern bool operator== (Key const & a, Key const & b);
extern bool operator!= (Key const & a, Key const & b);


}

#endif
