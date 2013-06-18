/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

/** @file  src/parse/pkl.h
 *  @brief Classes used to parse a PKL
 */

#include <boost/shared_ptr.hpp>
#include "../xml.h"

namespace libdcp {

namespace parse {

class PKLAsset
{
public:
	PKLAsset () {}
	PKLAsset (boost::shared_ptr<const cxml::Node>);

	std::string id;
	std::string annotation_text;
	std::string hash;
	int64_t size;
	std::string type;
	std::string original_file_name;
};

class PKL
{
public:
	PKL (std::string file);

	std::string id;
	std::string annotation_text;
	std::string issue_date;
	std::string issuer;
	std::string creator;
	std::list<boost::shared_ptr<PKLAsset> > assets;
};
	
}

}
