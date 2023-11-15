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


using std::make_shared;
using std::shared_ptr;
using std::string;
using std::vector;
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
	BOOST_REQUIRE_EQUAL (A._values.size(), 1U);
	BOOST_CHECK_EQUAL (A._values["foo"], "bar");

	A._values.clear ();
	B._values.clear ();

	A._values["foo"] = "bar";
	A._values["fred"] = "jim";

	B._values["foo"] = "hello";
	B._values["sheila"] = "baz";

	A.take_intersection (B);
	BOOST_CHECK_EQUAL (A._values.size(), 0U);
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
	BOOST_REQUIRE_EQUAL (A._values.size(), 1U);
	BOOST_CHECK_EQUAL (A._values["fred"], "jim");
}

/** Test dcp::order::Subtitle::pull_fonts */
BOOST_AUTO_TEST_CASE (pull_fonts_test1)
{
	auto root = make_shared<dcp::order::Part>(shared_ptr<dcp::order::Part>());
	auto sub1 = make_shared<dcp::order::Subtitle>(root, dcp::Time(), dcp::Time(), dcp::Time(), dcp::Time());
	root->children.push_back (sub1);
	auto text1 = make_shared<dcp::order::Text>(sub1, dcp::HAlign::CENTER, 0, dcp::VAlign::TOP, 0, 0, dcp::Direction::LTR, std::vector<dcp::Ruby>());
	sub1->children.push_back (text1);
	text1->font._values["font"] = "Inconsolata";
	text1->font._values["size"] = "42";

	dcp::SubtitleAsset::pull_fonts (root);

	BOOST_REQUIRE_EQUAL (sub1->font._values.size(), 2U);
	BOOST_CHECK_EQUAL (sub1->font._values["font"], "Inconsolata");
	BOOST_CHECK_EQUAL (sub1->font._values["size"], "42");
	BOOST_CHECK_EQUAL (text1->font._values.size(), 0U);
}

/** Test dcp::order::Subtitle::pull_fonts */
BOOST_AUTO_TEST_CASE (pull_fonts_test2)
{
	auto root = make_shared<dcp::order::Part>(shared_ptr<dcp::order::Part> ());
	auto sub1 = make_shared<dcp::order::Subtitle>(root, dcp::Time(), dcp::Time(), dcp::Time(), dcp::Time());
	root->children.push_back (sub1);
	auto text1 = make_shared<dcp::order::Text>(sub1, dcp::HAlign::CENTER, 0, dcp::VAlign::TOP, 0, 0, dcp::Direction::LTR, std::vector<dcp::Ruby>());
	sub1->children.push_back (text1);
	text1->font._values["font"] = "Inconsolata";
	text1->font._values["size"] = "42";
	auto text2 = make_shared<dcp::order::Text>(sub1, dcp::HAlign::CENTER, 0, dcp::VAlign::TOP, 0, 0, dcp::Direction::LTR, std::vector<dcp::Ruby>());
	sub1->children.push_back (text2);
	text2->font._values["font"] = "Inconsolata";
	text2->font._values["size"] = "48";

	dcp::SubtitleAsset::pull_fonts (root);

	BOOST_REQUIRE_EQUAL (sub1->font._values.size(), 1U);
	BOOST_CHECK_EQUAL (sub1->font._values["font"], "Inconsolata");
	BOOST_REQUIRE_EQUAL (text1->font._values.size(), 1U);
	BOOST_CHECK_EQUAL (text1->font._values["size"], "42");
	BOOST_REQUIRE_EQUAL (text2->font._values.size(), 1U);
	BOOST_CHECK_EQUAL (text2->font._values["size"], "48");
}

/** Test dcp::order::Subtitle::pull_fonts */
BOOST_AUTO_TEST_CASE (pull_fonts_test3)
{
	auto root = make_shared<dcp::order::Part>(shared_ptr<dcp::order::Part> ());
	auto sub1 = make_shared<dcp::order::Subtitle>(root, dcp::Time(), dcp::Time(), dcp::Time(), dcp::Time());
	root->children.push_back (sub1);
	auto text1 = make_shared<dcp::order::Text>(sub1, dcp::HAlign::CENTER, 0, dcp::VAlign::TOP, 0, 0, dcp::Direction::LTR, std::vector<dcp::Ruby>());
	sub1->children.push_back (text1);
	dcp::order::Font font;
	font._values["font"] = "Inconsolata";
	font._values["size"] = "42";
	auto string1 = make_shared<dcp::order::String>(text1, font, "Hello world", 0);
	text1->children.push_back (string1);

	dcp::SubtitleAsset::pull_fonts (root);

	BOOST_REQUIRE_EQUAL (sub1->font._values.size(), 2U);
	BOOST_CHECK_EQUAL (sub1->font._values["font"], "Inconsolata");
	BOOST_CHECK_EQUAL (sub1->font._values["size"], "42");
}


/* Check that subtitle XML is prettily formatted without inserting any white space into
 * <Text> node, which I think has the potential to alter appearance.
 */
BOOST_AUTO_TEST_CASE (format_xml_test1)
{
	xmlpp::Document doc;
	auto root = doc.create_root_node("Foo");
	root->add_child("Empty");
	root->add_child("Text")->add_child_text("Hello world");
	root->add_child("Font")->add_child("Text")->add_child_text("Say what");
	auto fred = root->add_child("Text")->add_child("Font");
	fred->set_attribute("bob", "job");
	fred->add_child_text("Fred");
	fred->add_child("Text")->add_child_text("Jim");
	fred->add_child_text("Sheila");
	BOOST_REQUIRE_EQUAL (dcp::SubtitleAsset::format_xml(doc, make_pair(string{}, string{"fred"})),
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<Foo xmlns=\"fred\">\n"
"  <Empty/>\n"
"  <Text>Hello world</Text>\n"
"  <Font>\n"
"    <Text>Say what</Text>\n"
"  </Font>\n"
"  <Text><Font bob=\"job\">Fred<Text>Jim</Text>Sheila</Font></Text>\n"
"</Foo>\n");
}


BOOST_AUTO_TEST_CASE (format_xml_test2)
{
	xmlpp::DomParser parser;
	auto path = private_test / "DKH_UT_EN20160601def.xml";
	parser.parse_file(path.string().c_str());
	auto document = parser.get_document();
	check_xml (dcp::file_to_string(private_test / "DKH_UT_EN20160601def.reformatted.xml"), dcp::SubtitleAsset::format_xml(*document, {}), {});
}


BOOST_AUTO_TEST_CASE (format_xml_entities_test)
{
	xmlpp::Document doc;
	auto root = doc.create_root_node("Foo");
	root->add_child("Bar")->add_child_text("Don't panic &amp; xml \"is\" 'great' & < > —");
	BOOST_REQUIRE_EQUAL(dcp::SubtitleAsset::format_xml(doc, {}),
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<Foo>\n"
"  <Bar>Don't panic &amp;amp; xml \"is\" 'great' &amp; &lt; &gt; —</Bar>\n"
"</Foo>\n");
}


BOOST_AUTO_TEST_CASE(ruby_round_trip_test)
{
	dcp::InteropSubtitleAsset asset("test/data/ruby1.xml");
	check_xml(dcp::file_to_string("test/data/ruby1.xml"), asset.xml_as_string(), {}, false);
}


