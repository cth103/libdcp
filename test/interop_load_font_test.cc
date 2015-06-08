/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#include "interop_load_font_node.h"
#include <libcxml/cxml.h>
#include <libxml++/libxml++.h>
#include <boost/test/unit_test.hpp>

/** Test dcp::InteropLoadFont's simple constructor */
BOOST_AUTO_TEST_CASE (interop_load_font_test1)
{
	dcp::InteropLoadFontNode lf ("my-great-id", "my-great-uri");
	BOOST_CHECK_EQUAL (lf.id, "my-great-id");
	BOOST_CHECK_EQUAL (lf.uri, "my-great-uri");
}

/** Test dcp::InteropLoadFont's XML constructor */
BOOST_AUTO_TEST_CASE (interop_load_font_test2)
{
	xmlpp::Document doc;
	xmlpp::Element* text = doc.create_root_node("Font");

	text->set_attribute("Id", "my-great-id");
	text->set_attribute("URI", "my-great-uri");
	dcp::InteropLoadFontNode lf (cxml::ConstNodePtr (new cxml::Node (text)));

	BOOST_CHECK_EQUAL (lf.id, "my-great-id");
}

/** As per _test2 but with another capitalisation of ID */
BOOST_AUTO_TEST_CASE (interop_load_font_test3)
{
	xmlpp::Document doc;
	xmlpp::Element* text = doc.create_root_node("Font");

	text->set_attribute("ID", "my-great-id");
	text->set_attribute("URI", "my-great-uri");
	dcp::InteropLoadFontNode lf (cxml::ConstNodePtr (new cxml::Node (text)));

	BOOST_CHECK_EQUAL (lf.id, "my-great-id");
}

/** Test operator== and operator!= */
BOOST_AUTO_TEST_CASE (interop_load_font_test4)
{
	dcp::InteropLoadFontNode A ("my-create-id", "my-great-uri");
	dcp::InteropLoadFontNode B ("my-create-id", "my-great-uri");
	dcp::InteropLoadFontNode C ("my-create-id", "another-great-uri");

	BOOST_CHECK (A == B);
	BOOST_CHECK (B != C);
}
