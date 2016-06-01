/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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
*/

#include "smpte_load_font_node.h"
#include <libcxml/cxml.h>
#include <libxml++/libxml++.h>
#include <boost/test/unit_test.hpp>

/** Test dcp::SMPTELoadFontNode */
BOOST_AUTO_TEST_CASE (smpte_load_font_test1)
{
	xmlpp::Document doc;
	xmlpp::Element* text = doc.create_root_node("Font");

	text->set_attribute ("ID", "my-great-id");
	text->add_child_text ("urn:uuid:my-great-urn");
	dcp::SMPTELoadFontNode lf (cxml::ConstNodePtr (new cxml::Node (text)));

	BOOST_CHECK_EQUAL (lf.id, "my-great-id");
	BOOST_CHECK_EQUAL (lf.urn, "my-great-urn");
}
