/*
    Copyright (C) 2015-2018 Carl Hetherington <cth@carlh.net>

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

#include "interop_subtitle_asset.h"
#include "smpte_subtitle_asset.h"
#include "subtitle_string.h"
#include "subtitle_image.h"
#include "subtitle_asset_internal.h"
#include "reel_subtitle_asset.h"
#include "reel.h"
#include "cpl.h"
#include "dcp.h"
#include "test.h"
#include "util.h"
#include <boost/test/unit_test.hpp>

using std::list;
using std::string;
using boost::shared_ptr;

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
	shared_ptr<dcp::order::Part> root (new dcp::order::Part (shared_ptr<dcp::order::Part> ()));
	shared_ptr<dcp::order::Subtitle> sub1 (new dcp::order::Subtitle (root, dcp::Time(), dcp::Time(), dcp::Time(), dcp::Time()));
	root->children.push_back (sub1);
	shared_ptr<dcp::order::Text> text1 (new dcp::order::Text (sub1, dcp::HALIGN_CENTER, 0, dcp::VALIGN_TOP, 0, dcp::DIRECTION_LTR));
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
	shared_ptr<dcp::order::Part> root (new dcp::order::Part (shared_ptr<dcp::order::Part> ()));
	shared_ptr<dcp::order::Subtitle> sub1 (new dcp::order::Subtitle (root, dcp::Time(), dcp::Time(), dcp::Time(), dcp::Time()));
	root->children.push_back (sub1);
	shared_ptr<dcp::order::Text> text1 (new dcp::order::Text (sub1, dcp::HALIGN_CENTER, 0, dcp::VALIGN_TOP, 0, dcp::DIRECTION_LTR));
	sub1->children.push_back (text1);
	text1->font._values["font"] = "Inconsolata";
	text1->font._values["size"] = "42";
	shared_ptr<dcp::order::Text> text2 (new dcp::order::Text (sub1, dcp::HALIGN_CENTER, 0, dcp::VALIGN_TOP, 0, dcp::DIRECTION_LTR));
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
	shared_ptr<dcp::order::Part> root (new dcp::order::Part (shared_ptr<dcp::order::Part> ()));
	shared_ptr<dcp::order::Subtitle> sub1 (new dcp::order::Subtitle (root, dcp::Time(), dcp::Time(), dcp::Time(), dcp::Time()));
	root->children.push_back (sub1);
	shared_ptr<dcp::order::Text> text1 (new dcp::order::Text (sub1, dcp::HALIGN_CENTER, 0, dcp::VALIGN_TOP, 0, dcp::DIRECTION_LTR));
	sub1->children.push_back (text1);
	dcp::order::Font font;
	font._values["font"] = "Inconsolata";
	font._values["size"] = "42";
	shared_ptr<dcp::order::String> string1 (new dcp::order::String (text1, font, "Hello world"));
	text1->children.push_back (string1);

	dcp::SubtitleAsset::pull_fonts (root);

	BOOST_REQUIRE_EQUAL (sub1->font._values.size(), 2);
	BOOST_CHECK_EQUAL (sub1->font._values["font"], "Inconsolata");
	BOOST_CHECK_EQUAL (sub1->font._values["size"], "42");
}

/** Write some subtitle content as Interop XML and check that it is right */
BOOST_AUTO_TEST_CASE (write_interop_subtitle_test)
{
	dcp::InteropSubtitleAsset c;
	c.set_reel_number ("1");
	c.set_language ("EN");
	c.set_movie_title ("Test");

	c.add (
		shared_ptr<dcp::Subtitle> (
			new dcp::SubtitleString (
				string ("Frutiger"),
				false,
				false,
				false,
				dcp::Colour (255, 255, 255),
				48,
				1.0,
				dcp::Time (0, 4,  9, 22, 24),
				dcp::Time (0, 4, 11, 22, 24),
				0,
				dcp::HALIGN_CENTER,
				0.8,
				dcp::VALIGN_TOP,
				dcp::DIRECTION_LTR,
				"Hello world",
				dcp::NONE,
				dcp::Colour (0, 0, 0),
				dcp::Time (0, 0, 0, 0, 24),
				dcp::Time (0, 0, 0, 0, 24)
				)
			)
		);

	c.add (
		shared_ptr<dcp::Subtitle> (
			new dcp::SubtitleString (
				boost::optional<string> (),
				true,
				true,
				true,
				dcp::Colour (128, 0, 64),
				91,
				1.0,
				dcp::Time (5, 41,  0, 21, 24),
				dcp::Time (6, 12, 15, 21, 24),
				0,
				dcp::HALIGN_CENTER,
				0.4,
				dcp::VALIGN_BOTTOM,
				dcp::DIRECTION_LTR,
				"What's going on",
				dcp::BORDER,
				dcp::Colour (1, 2, 3),
				dcp::Time (1, 2, 3, 4, 24),
				dcp::Time (5, 6, 7, 8, 24)
				)
			)
		);

	c._id = "a6c58cff-3e1e-4b38-acec-a42224475ef6";

	check_xml (
		"<DCSubtitle Version=\"1.0\">"
		  "<SubtitleID>a6c58cff-3e1e-4b38-acec-a42224475ef6</SubtitleID>"
		  "<MovieTitle>Test</MovieTitle>"
		  "<ReelNumber>1</ReelNumber>"
		  "<Language>EN</Language>"
		  "<Font AspectAdjust=\"1.0\" Color=\"FFFFFFFF\" Effect=\"none\" EffectColor=\"FF000000\" Id=\"Frutiger\" Italic=\"no\" Script=\"normal\" Size=\"48\" Underlined=\"no\" Weight=\"normal\">"
		    "<Subtitle SpotNumber=\"1\" TimeIn=\"00:04:09:229\" TimeOut=\"00:04:11:229\" FadeUpTime=\"0\" FadeDownTime=\"0\">"
		      "<Text VAlign=\"top\" VPosition=\"80\">Hello world</Text>"
		    "</Subtitle>"
		  "</Font>"
		  "<Font AspectAdjust=\"1.0\" Color=\"FF800040\" Effect=\"border\" EffectColor=\"FF010203\" Italic=\"yes\" Script=\"normal\" Size=\"91\" Underlined=\"yes\" Weight=\"bold\">"
		    "<Subtitle SpotNumber=\"2\" TimeIn=\"05:41:00:219\" TimeOut=\"06:12:15:219\" FadeUpTime=\"930792\" FadeDownTime=\"4591834\">"
		      "<Text VAlign=\"bottom\" VPosition=\"40\">What's going on</Text>"
		    "</Subtitle>"
		  "</Font>"
		"</DCSubtitle>",
		c.xml_as_string (),
		list<string> ()
		);
}

/** Write some subtitle content as Interop XML and check that it is right.
 *  This test includes some horizontal alignment.
 */
BOOST_AUTO_TEST_CASE (write_interop_subtitle_test2)
{
	dcp::InteropSubtitleAsset c;
	c.set_reel_number ("1");
	c.set_language ("EN");
	c.set_movie_title ("Test");

	c.add (
		shared_ptr<dcp::Subtitle> (
			new dcp::SubtitleString (
				string ("Frutiger"),
				false,
				false,
				false,
				dcp::Colour (255, 255, 255),
				48,
				1.0,
				dcp::Time (0, 4,  9, 22, 24),
				dcp::Time (0, 4, 11, 22, 24),
				-0.2,
				dcp::HALIGN_CENTER,
				0.8,
				dcp::VALIGN_TOP,
				dcp::DIRECTION_LTR,
				"Hello world",
				dcp::NONE,
				dcp::Colour (0, 0, 0),
				dcp::Time (0, 0, 0, 0, 24),
				dcp::Time (0, 0, 0, 0, 24)
				)
			)
		);

	c.add (
		shared_ptr<dcp::Subtitle> (
			new dcp::SubtitleString (
				boost::optional<string> (),
				true,
				true,
				true,
				dcp::Colour (128, 0, 64),
				91,
				1.0,
				dcp::Time (5, 41,  0, 21, 24),
				dcp::Time (6, 12, 15, 21, 24),
				-0.2,
				dcp::HALIGN_CENTER,
				0.4,
				dcp::VALIGN_BOTTOM,
				dcp::DIRECTION_LTR,
				"What's going on",
				dcp::BORDER,
				dcp::Colour (1, 2, 3),
				dcp::Time (1, 2, 3, 4, 24),
				dcp::Time (5, 6, 7, 8, 24)
				)
			)
		);

	c._id = "a6c58cff-3e1e-4b38-acec-a42224475ef6";

	check_xml (
		"<DCSubtitle Version=\"1.0\">"
		  "<SubtitleID>a6c58cff-3e1e-4b38-acec-a42224475ef6</SubtitleID>"
		  "<MovieTitle>Test</MovieTitle>"
		  "<ReelNumber>1</ReelNumber>"
		  "<Language>EN</Language>"
		  "<Font AspectAdjust=\"1.0\" Color=\"FFFFFFFF\" Effect=\"none\" EffectColor=\"FF000000\" Id=\"Frutiger\" Italic=\"no\" Script=\"normal\" Size=\"48\" Underlined=\"no\" Weight=\"normal\">"
		    "<Subtitle SpotNumber=\"1\" TimeIn=\"00:04:09:229\" TimeOut=\"00:04:11:229\" FadeUpTime=\"0\" FadeDownTime=\"0\">"
		      "<Text HPosition=\"-20\" VAlign=\"top\" VPosition=\"80\">Hello world</Text>"
		    "</Subtitle>"
		  "</Font>"
		  "<Font AspectAdjust=\"1.0\" Color=\"FF800040\" Effect=\"border\" EffectColor=\"FF010203\" Italic=\"yes\" Script=\"normal\" Size=\"91\" Underlined=\"yes\" Weight=\"bold\">"
		    "<Subtitle SpotNumber=\"2\" TimeIn=\"05:41:00:219\" TimeOut=\"06:12:15:219\" FadeUpTime=\"930792\" FadeDownTime=\"4591834\">"
		      "<Text HPosition=\"-20\" VAlign=\"bottom\" VPosition=\"40\">What's going on</Text>"
		    "</Subtitle>"
		  "</Font>"
		"</DCSubtitle>",
		c.xml_as_string (),
		list<string> ()
		);
}

/* Write some subtitle content as Interop XML using bitmaps and check that it is right */
BOOST_AUTO_TEST_CASE (write_interop_subtitle_test3)
{
	shared_ptr<dcp::InteropSubtitleAsset> c (new dcp::InteropSubtitleAsset());
	c->set_reel_number ("1");
	c->set_language ("EN");
	c->set_movie_title ("Test");

	c->add (
		shared_ptr<dcp::Subtitle> (
			new dcp::SubtitleImage (
				dcp::Data ("test/data/sub.png"),
				dcp::Time (0, 4,  9, 22, 24),
				dcp::Time (0, 4, 11, 22, 24),
				0,
				dcp::HALIGN_CENTER,
				0.8,
				dcp::VALIGN_TOP,
				dcp::Time (0, 0, 0, 0, 24),
				dcp::Time (0, 0, 0, 0, 24)
				)
			)
		);

	c->_id = "a6c58cff-3e1e-4b38-acec-a42224475ef6";
	boost::filesystem::create_directories ("build/test/write_interop_subtitle_test3");
	c->write ("build/test/write_interop_subtitle_test3/subs.xml");

	shared_ptr<dcp::Reel> reel (new dcp::Reel());
	reel->add(shared_ptr<dcp::ReelSubtitleAsset>(new dcp::ReelSubtitleAsset(c, dcp::Fraction(24, 1), 6046, 0)));

	dcp::XMLMetadata xml_meta;
	xml_meta.issue_date = "2018-09-02T04:45:18+00:00";
	xml_meta.issuer = "libdcp";
	xml_meta.creator = "libdcp";
	xml_meta.annotation_text = "Created by libdcp";

	shared_ptr<dcp::CPL> cpl (new dcp::CPL ("My film", dcp::FEATURE));
	cpl->add (reel);
	cpl->set_metadata (xml_meta);
	cpl->set_content_version_label_text ("foo");

	dcp::DCP dcp ("build/test/write_interop_subtitle_test3");
	dcp.add (cpl);
	dcp.write_xml (dcp::INTEROP, xml_meta);

	check_xml (
		dcp::file_to_string("test/ref/write_interop_subtitle_test3/subs.xml"),
		dcp::file_to_string("build/test/write_interop_subtitle_test3/subs.xml"),
		list<string>()
		);
	check_file ("build/test/write_interop_subtitle_test3/ef5c6baa-be2d-4f86-9f15-b1acc792ee8b.png", "test/data/sub.png");

	check_xml (
		dcp::file_to_string("test/ref/write_interop_subtitle_test3/ASSETMAP"),
		dcp::file_to_string("build/test/write_interop_subtitle_test3/ASSETMAP"),
		list<string>()
		);

	check_xml (
		dcp::file_to_string("test/ref/write_interop_subtitle_test3/pkl.xml"),
		dcp::file_to_string("build/test/write_interop_subtitle_test3/pkl_f5aab304-8145-44e3-a265-aa8d8812d8a2.xml"),
		list<string>()
		);
}

