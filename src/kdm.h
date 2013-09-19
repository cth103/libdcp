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

#ifndef LIBDCP_KDM_H
#define LIBDCP_KDM_H

#include <boost/filesystem.hpp>
#include "key.h"

namespace libdcp {

/** A single key for encrypting or decrypting an MXF.  One or more of these
 *  are delivered in a KDM.
 */
class KDMKey
{
public:
	KDMKey (unsigned char const *, int);

	std::string structure_id () const {
		return _structure_id;
	}
	
	std::string signer_thumbprint () const {
		return _signer_thumbprint;
	}
	
	std::string cpl_id () const {
		return _cpl_id;
	}
	
	std::string key_type () const {
		return _key_type;
	}

	std::string key_id () const {
		return _key_id;
	}
	
	std::string not_valid_before () const {
		return _not_valid_before;
	}
	
	std::string not_valid_after () const {
		return _not_valid_after;
	}

	Key key () const {
		return _key;
	}
	
private:
	std::string get (unsigned char const **, int) const;
	std::string get_uuid (unsigned char const **, int) const;
	
	std::string _structure_id;
	std::string _signer_thumbprint;
	std::string _cpl_id;
	std::string _not_valid_before;
	std::string _not_valid_after;
	std::string _key_type;
	std::string _key_id;
	Key _key;
};

class KDM
{
public:
	KDM (boost::filesystem::path, boost::filesystem::path);

	std::list<KDMKey> keys () const {
		return _keys;
	}

private:
	std::list<KDMKey> _keys;
};


}

#endif
