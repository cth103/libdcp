/*
    Copyright (C) 2014-2019 Carl Hetherington <cth@carlh.net>

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


#include "interop_load_font_node.h"
#include "warnings.h"
#include <libcxml/cxml.h>
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <boost/test/unit_test.hpp>


using std::make_shared;


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
	dcp::InteropLoadFontNode lf (make_shared<cxml::Node>(text));

	BOOST_CHECK_EQUAL (lf.id, "my-great-id");
}

/** As per _test2 but with another capitalisation of ID */
BOOST_AUTO_TEST_CASE (interop_load_font_test3)
{
	xmlpp::Document doc;
	xmlpp::Element* text = doc.create_root_node("Font");

	text->set_attribute("ID", "my-great-id");
	text->set_attribute("URI", "my-great-uri");
	dcp::InteropLoadFontNode lf (make_shared<cxml::Node>(text));

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
