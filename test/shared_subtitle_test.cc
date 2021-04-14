/*
    Copyright (C) 2015-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  test/shared_subtitle_test.cc
 *  @brief Tests of the code that is shared between Interop and SMPTE subtitles.
 */


#include "interop_subtitle_asset.h"
#include "smpte_subtitle_asset.h"
#include "subtitle_string.h"
#include "subtitle_image.h"
#include "subtitle_asset_internal.h"
#include "reel_interop_subtitle_asset.h"
#include "reel.h"
#include "cpl.h"
#include "dcp.h"
#include "test.h"
#include "util.h"
#include <boost/test/unit_test.hpp>


using std::string;
using std::shared_ptr;
using std::vector;
using std::make_shared;
using boost::optional;


/** Test dcp::order::Font::take_intersection */
BOOST_AUTO_TEST_CASE (take_intersection_test)
{
	dcp::order::Font A;
	A._values["foo"] = "bar";
	A._values["fred"] = "jim";

	dcp::order::Font B;
	B._values["foo"] = "bar";
	B._values["sheila"] = "baz";

	A.take_intersection (B);
	BOOST_REQUIRE_EQUAL (A._values.size(), 1);
	BOOST_CHECK_EQUAL (A._values["foo"], "bar");

	A._values.clear ();
	B._values.clear ();

	A._values["foo"] = "bar";
	A._values["fred"] = "jim";

	B._values["foo"] = "hello";
	B._values["sheila"] = "baz";

	A.take_intersection (B);
	BOOST_CHECK_EQUAL (A._values.size(), 0);
}

/** Test dcp::order::Font::take_difference */
BOOST_AUTO_TEST_CASE (take_difference_test)
{
	dcp::order::Font A;
	A._values["foo"] = "bar";
	A._values["fred"] = "jim";

	dcp::order::Font B;
	B._values["foo"] = "bar";
	B._values["sheila"] = "baz";

	A.take_difference (B);
	BOOST_REQUIRE_EQUAL (A._values.size(), 1);
	BOOST_CHECK_EQUAL (A._values["fred"], "jim");
}

/** Test dcp::order::Subtitle::pull_fonts */
BOOST_AUTO_TEST_CASE (pull_fonts_test1)
{
	auto root = make_shared<dcp::order::Part>(shared_ptr<dcp::order::Part>());
	auto sub1 = make_shared<dcp::order::Subtitle>(root, dcp::Time(), dcp::Time(), dcp::Time(), dcp::Time());
	root->children.push_back (sub1);
	auto text1 = make_shared<dcp::order::Text>(sub1, dcp::HAlign::CENTER, 0, dcp::VAlign::TOP, 0, dcp::Direction::LTR);
	sub1->children.push_back (text1);
	text1->font._values["font"] = "Inconsolata";
	text1->font._values["size"] = "42";

	dcp::SubtitleAsset::pull_fonts (root);

	BOOST_REQUIRE_EQUAL (sub1->font._values.size(), 2);
	BOOST_CHECK_EQUAL (sub1->font._values["font"], "Inconsolata");
	BOOST_CHECK_EQUAL (sub1->font._values["size"], "42");
	BOOST_CHECK_EQUAL (text1->font._values.size(), 0);
}

/** Test dcp::order::Subtitle::pull_fonts */
BOOST_AUTO_TEST_CASE (pull_fonts_test2)
{
	auto root = make_shared<dcp::order::Part>(shared_ptr<dcp::order::Part> ());
	auto sub1 = make_shared<dcp::order::Subtitle>(root, dcp::Time(), dcp::Time(), dcp::Time(), dcp::Time());
	root->children.push_back (sub1);
	auto text1 = make_shared<dcp::order::Text>(sub1, dcp::HAlign::CENTER, 0, dcp::VAlign::TOP, 0, dcp::Direction::LTR);
	sub1->children.push_back (text1);
	text1->font._values["font"] = "Inconsolata";
	text1->font._values["size"] = "42";
	auto text2 = make_shared<dcp::order::Text>(sub1, dcp::HAlign::CENTER, 0, dcp::VAlign::TOP, 0, dcp::Direction::LTR);
	sub1->children.push_back (text2);
	text2->font._values["font"] = "Inconsolata";
	text2->font._values["size"] = "48";

	dcp::SubtitleAsset::pull_fonts (root);

	BOOST_REQUIRE_EQUAL (sub1->font._values.size(), 1);
	BOOST_CHECK_EQUAL (sub1->font._values["font"], "Inconsolata");
	BOOST_REQUIRE_EQUAL (text1->font._values.size(), 1);
	BOOST_CHECK_EQUAL (text1->font._values["size"], "42");
	BOOST_REQUIRE_EQUAL (text2->font._values.size(), 1);
	BOOST_CHECK_EQUAL (text2->font._values["size"], "48");
}

/** Test dcp::order::Subtitle::pull_fonts */
BOOST_AUTO_TEST_CASE (pull_fonts_test3)
{
	auto root = make_shared<dcp::order::Part>(shared_ptr<dcp::order::Part> ());
	auto sub1 = make_shared<dcp::order::Subtitle>(root, dcp::Time(), dcp::Time(), dcp::Time(), dcp::Time());
	root->children.push_back (sub1);
	auto text1 = make_shared<dcp::order::Text>(sub1, dcp::HAlign::CENTER, 0, dcp::VAlign::TOP, 0, dcp::Direction::LTR);
	sub1->children.push_back (text1);
	dcp::order::Font font;
	font._values["font"] = "Inconsolata";
	font._values["size"] = "42";
	auto string1 = make_shared<dcp::order::String>(text1, font, "Hello world");
	text1->children.push_back (string1);

	dcp::SubtitleAsset::pull_fonts (root);

	BOOST_REQUIRE_EQUAL (sub1->font._values.size(), 2);
	BOOST_CHECK_EQUAL (sub1->font._values["font"], "Inconsolata");
	BOOST_CHECK_EQUAL (sub1->font._values["size"], "42");
}

