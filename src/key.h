/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

/** @file  src/key.h
 *  @brief Class to hold a key for encrypting MXFs.
 */

#ifndef LIBDCP_KEY_H
#define LIBDCP_KEY_H

#include <stdint.h>

namespace libdcp {

/** @class Key
 *  @brief A key for encrypting MXFs.
 */
class Key
{
public:
	/** Create a new, random key */
	Key ();

	/** Create a Key from a raw key value */
	Key (uint8_t const *);

	/** Create a Key from a hex key value */
	Key (std::string);

	Key (Key const &);
	~Key ();

	Key& operator= (Key const &);

	/** @return Raw key value */
	uint8_t const * value () const {
		return _value;
	}

	/** @return Key value as a hexadecimal string */
	std::string hex () const;

private:
	/** Raw key value */
	uint8_t* _value;
};

extern bool operator== (Key const & a, Key const & b);
extern bool operator!= (Key const & a, Key const & b);

}

#endif
