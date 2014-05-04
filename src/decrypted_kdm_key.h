/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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

/** @file  src/decrypted_kdm_key.h
 *  @brief DecryptedKDMKey class
 */

#ifndef LIBDCP_DECRYPTED_KDM_KEY_H
#define LIBDCP_DECRYPTED_KDM_KEY_H

#include "key.h"

namespace dcp {

/** @class DecryptedKDMKey
 *  @brief An un- or de-crypted key from a KDM.
 */
class DecryptedKDMKey
{
public:
	DecryptedKDMKey (std::string type, std::string id, Key key, std::string cpl_id)
		: _type (type)
		, _id (id)
		, _key (key)
		, _cpl_id (cpl_id)
	{}

	std::string type () const {
		return _type;
	}

	std::string id () const {
		return _id;
	}

	Key key () const {
		return _key;
	}

	std::string cpl_id () const {
		return _cpl_id;
	}

private:	
	std::string _type;
	std::string _id;
	Key _key;
	std::string _cpl_id;
};

bool operator== (DecryptedKDMKey const &, DecryptedKDMKey const &);

}

#endif
