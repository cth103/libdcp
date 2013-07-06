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

namespace libdcp {

class KDMCipher
{
public:
	KDMCipher (unsigned char const *, int);

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
	
	std::string key_string () const {
		return _key_string;
	}

	unsigned char const * key_raw () const {
		return _key_raw;
	}
	
private:
	std::string get (unsigned char const **, int) const;
	std::string get_uuid (unsigned char const **, int) const;
	std::string get_hex (unsigned char const **, int) const;
	
	std::string _structure_id;
	std::string _signer_thumbprint;
	std::string _cpl_id;
	std::string _key_type;
	std::string _key_id;
	std::string _not_valid_before;
	std::string _not_valid_after;
	std::string _key_string;
	unsigned char _key_raw[16];
};

class KDM
{
public:
	KDM (boost::filesystem::path, boost::filesystem::path);

	std::list<KDMCipher> ciphers () const {
		return _ciphers;
	}

private:
	std::list<KDMCipher> _ciphers;
};


}

#endif
