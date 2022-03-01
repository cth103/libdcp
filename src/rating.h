/*
    Copyright (C) 2012-2022 Carl Hetherington <cth@carlh.net>

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


#ifndef LIBDCP_RATING_H
#define LIBDCP_RATING_H


#include "warnings.h"
#include <libcxml/cxml.h>
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <string>


namespace dcp {


class Rating
{
public:
	Rating (std::string agency_, std::string label_)
		: agency (agency_)
		, label (label_)
	{}

	explicit Rating (cxml::ConstNodePtr node);

	void as_xml (xmlpp::Element* parent) const;

	/** URI of the agency issuing the rating */
	std::string agency;
	/** Rating (e.g. PG, PG-13, 12A etc) */
	std::string label;
};


extern bool operator== (Rating const & a, Rating const & b);


class RatingSystem
{
public:
	RatingSystem (std::string agency_, std::string name_, std::string country_and_region_names_, std::string country_code_)
		: agency(agency_)
		, name(name_)
		, country_and_region_names(country_and_region_names_)
		, country_code(country_code_)
	{}

	/** URI of the agency issuing the rating */
	std::string agency;
	/** Name of the rating system */
	std::string name;
	/** Country name, possibly followed by a slash and a region name */
	std::string country_and_region_names;
	/** Country code */
	std::string country_code;

	std::vector<Rating> ratings;
};


std::vector<RatingSystem> rating_systems();

void load_rating_list(boost::filesystem::path ratings_file);

}


#endif

