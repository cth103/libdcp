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

#include "local_time.h"
#include <boost/filesystem.hpp>
#include <boost/date_time/local_time/local_time.hpp>

namespace cxml {
	class Node;
}

namespace dcp {

namespace data {
	class EncryptedKDMData;
}

class Signer;	
class Certificate;

class EncryptedKDM
{
public:
	/** Read a KDM from an XML file */
	EncryptedKDM (boost::filesystem::path file);

	/** Construct an EncryptedKDM from a set of details */
	EncryptedKDM (
		boost::shared_ptr<const Signer> signer,
		boost::shared_ptr<const Certificate> recipient,
		std::string device_list_description,
		std::string cpl_id,
		std::string cpl_content_title_text,
		LocalTime _not_valid_before,
		LocalTime _not_valid_after,
		std::list<std::pair<std::string, std::string> > key_ids,
		std::list<std::string> keys
		);

	EncryptedKDM (EncryptedKDM const & kdm);
	EncryptedKDM & operator= (EncryptedKDM const &);
	~EncryptedKDM ();

	void as_xml (boost::filesystem::path) const;
	std::string as_xml () const;

	std::list<std::string> keys () const;
	
private:
	data::EncryptedKDMData* _data;
};

}
