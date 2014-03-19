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

#include "key.h"
#include "local_time.h"
#include "decrypted_kdm_key.h"
#include <boost/filesystem.hpp>

namespace dcp {

class DecryptedKDMKey;
class EncryptedKDM;
class Signer;
class Certificate;
class CPL;

class DecryptedKDM
{
public:
	/** @param kdm Encrypted KDM.
	 *  @param private_key Private key file name.
	 */
	DecryptedKDM (EncryptedKDM const & kdm, boost::filesystem::path private_key);

	DecryptedKDM (
		boost::shared_ptr<const CPL> cpl,
		LocalTime not_valid_before,
		LocalTime not_valid_after,
		std::string annotation_text,
		std::string content_title_text,
		std::string issue_date
		);

	void add_key (std::string type, std::string id, Key key);
	EncryptedKDM encrypt (boost::shared_ptr<const Signer>, boost::shared_ptr<const Certificate>) const;

	std::list<DecryptedKDMKey> keys () const {
		return _keys;
	}

private:
	LocalTime _not_valid_before;
	LocalTime _not_valid_after;
	std::string _annotation_text;
	std::string _content_title_text;
	std::string _issue_date;
	std::list<DecryptedKDMKey> _keys;
};

}
