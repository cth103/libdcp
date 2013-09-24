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
#include <boost/scoped_ptr.hpp>
#include "key.h"
#include "metadata.h"

namespace libdcp {

namespace xml {
	class DCinemaSecurityMessage;
};

class Signer;
class Certificate;
class CPL;

/** A single key for encrypting or decrypting an MXF.  One or more of these
 *  are delivered in a KDM.
 */
class KDMKey
{
public:
	KDMKey (uint8_t const *, int);

	KDMKey (
		boost::shared_ptr<const Signer> signer,
		std::string cpl_id, std::string key_id, boost::posix_time::ptime from, boost::posix_time::ptime until, Key key
		);
	
	KDMKey (KDMKey const &);

	KDMKey& operator= (KDMKey const &);

	std::string cpl_id () const {
		return _cpl_id;
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

	std::string encrypted_base64 (boost::shared_ptr<const Certificate>) const;
	
private:
	void get (uint8_t *, uint8_t const **, int) const;
	std::string get (uint8_t const **, int) const;
	std::string get_uuid (uint8_t const **) const;
	void put (uint8_t **, uint8_t const *, int) const;
	void put (uint8_t **, std::string) const;
	void put_uuid (uint8_t **, std::string) const;
	
	uint8_t _signer_thumbprint[20];
	std::string _cpl_id;
	std::string _key_type;
	std::string _key_id;
	std::string _not_valid_before;
	std::string _not_valid_after;
	Key _key;
};

class KDM
{
public:
	KDM (boost::filesystem::path, boost::filesystem::path);

	KDM (
		boost::shared_ptr<const CPL> cpl, boost::shared_ptr<const Signer>, boost::shared_ptr<const Certificate> recipient_cert,
		boost::posix_time::ptime not_valid_before, boost::posix_time::ptime not_valid_after,
		std::string annotation_text, std::string issue_date
		);

	std::list<KDMKey> keys () const {
		return _keys;
	}

	void as_xml (boost::filesystem::path) const;

private:
	std::string _message_id;
	std::list<KDMKey> _keys;

	boost::shared_ptr<xml::DCinemaSecurityMessage> xml_kdm;
};


}

#endif
