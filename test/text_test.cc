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

#include "text_node.h"
#include <libcxml/cxml.h>
#include <libxml++/libxml++.h>
#include <boost/test/unit_test.hpp>

/** Simple test of Text class parsing some XML */
BOOST_AUTO_TEST_CASE (text_test1)
{
	xmlpp::Document doc;
	xmlpp::Element* text = doc.create_root_node("Text");
	text->set_attribute("VPosition", "4.2");
	text->set_attribute("VAlign", "top");
	text->add_child_text("Hello world");

	dcp::TextNode t (cxml::NodePtr (new cxml::Node (text)), 250, "Id");
	BOOST_CHECK_CLOSE (t.v_position, 0.042, 0.001);
	BOOST_CHECK_EQUAL (t.v_align, dcp::VALIGN_TOP);
	BOOST_CHECK_EQUAL (t.text, "Hello world");
}

/** Similar to text_test1 but with different capitalisation of attribute names */
BOOST_AUTO_TEST_CASE (text_test2)
{
	xmlpp::Document doc;
	xmlpp::Element* text = doc.create_root_node("Text");
	text->set_attribute("Vposition", "4.2");
	text->set_attribute("Valign", "top");
	text->add_child_text("Hello world");

	dcp::TextNode t (cxml::NodePtr (new cxml::Node (text)), 250, "Id");
	BOOST_CHECK_CLOSE (t.v_position, 0.042, 0.001);
	BOOST_CHECK_EQUAL (t.v_align, dcp::VALIGN_TOP);
	BOOST_CHECK_EQUAL (t.text, "Hello world");
}
