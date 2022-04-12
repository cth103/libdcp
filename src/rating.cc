/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


#include "exceptions.h"
#include "file.h"
#include "rating.h"
#include "util.h"
#include <libcxml/cxml.h>
#include <boost/algorithm/string.hpp>


using std::string;
using std::vector;
using boost::algorithm::trim;
using boost::optional;
using namespace dcp;


static vector<RatingSystem> rating_systems_list;


Rating::Rating (cxml::ConstNodePtr node)
	: agency(node->string_child("Agency"))
	, label(node->string_child("Label"))
{
	node->done ();
}


void
Rating::as_xml (xmlpp::Element* parent) const
{
	parent->add_child("Agency")->add_child_text(agency);
	parent->add_child("Label")->add_child_text(label);
}


bool
dcp::operator== (Rating const & a, Rating const & b)
{
	return a.agency == b.agency && a.label == b.label;
}


vector<RatingSystem>
dcp::rating_systems()
{
	return rating_systems_list;
}


void
dcp::load_rating_list(boost::filesystem::path ratings_file)
{
	File f(ratings_file, "r");
	if (!f) {
		throw FileError ("Could not open ratings file", ratings_file, errno);
	}

	auto get_line_no_throw = [&f, ratings_file]() -> optional<string> {
		char buffer[512];
		char* r = f.gets(buffer, sizeof(buffer));
		if (r == 0) {
			return {};
		}
		string a = buffer;
		trim(a);
		return a;
	};

	auto get_line = [ratings_file, &get_line_no_throw]() {
		auto line = get_line_no_throw();
		if (!line) {
			throw FileError("Bad ratings file", ratings_file, -1);
		}
		return *line;
	};

	optional<string> agency;

	while (!f.eof()) {
		if (!agency) {
			agency = get_line();
		}
		auto name = get_line();
		auto country_and_region_names = get_line();
		auto country_code = get_line();

		RatingSystem system(*agency, name, country_and_region_names, country_code);
		while (!f.eof()) {
			auto rating = get_line_no_throw();
			if (!rating) {
				/* End of the file */
				break;
			}
			if (rating->substr(0, 4) == "http") {
				/* End of the system */
				agency = rating;
				break;
			}
			system.ratings.push_back(dcp::Rating(*agency, *rating));
		}

		rating_systems_list.push_back(system);
	}
}

