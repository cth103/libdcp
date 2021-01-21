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
	shared_ptr<dcp::order::Part> root (new dcp::order::Part (shared_ptr<dcp::order::Part> ()));
	shared_ptr<dcp::order::Subtitle> sub1 (new dcp::order::Subtitle (root, dcp::Time(), dcp::Time(), dcp::Time(), dcp::Time()));
	root->children.push_back (sub1);
	shared_ptr<dcp::order::Text> text1 (new dcp::order::Text (sub1, dcp::HAlign::CENTER, 0, dcp::VAlign::TOP, 0, dcp::Direction::LTR));
	sub1->children.push_back (text1);
	text1->font._values["font"] = "Inconsolata";
	text1->font._values["size"] = "42";
	shared_ptr<dcp::order::Text> text2 (new dcp::order::Text (sub1, dcp::HAlign::CENTER, 0, dcp::VAlign::TOP, 0, dcp::Direction::LTR));
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
	shared_ptr<dcp::order::Text> text1 (new dcp::order::Text (sub1, dcp::HAlign::CENTER, 0, dcp::VAlign::TOP, 0, dcp::Direction::LTR));
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
				dcp::HAlign::CENTER,
				0.8,
				dcp::VAlign::TOP,
				dcp::Direction::LTR,
				"Hello world",
				dcp::Effect::NONE,
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
				dcp::HAlign::CENTER,
				0.4,
				dcp::VAlign::BOTTOM,
				dcp::Direction::LTR,
				"What's going on",
				dcp::Effect::BORDER,
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
		vector<string>()
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
				dcp::HAlign::CENTER,
				0.8,
				dcp::VAlign::TOP,
				dcp::Direction::LTR,
				"Hello world",
				dcp::Effect::NONE,
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
				dcp::HAlign::CENTER,
				0.4,
				dcp::VAlign::BOTTOM,
				dcp::Direction::LTR,
				"What's going on",
				dcp::Effect::BORDER,
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
		vector<string>()
		);
}

/* Write some subtitle content as Interop XML using bitmaps and check that it is right */
BOOST_AUTO_TEST_CASE (write_interop_subtitle_test3)
{
	RNGFixer fix;

	shared_ptr<dcp::InteropSubtitleAsset> c (new dcp::InteropSubtitleAsset());
	c->set_reel_number ("1");
	c->set_language ("EN");
	c->set_movie_title ("Test");

	c->add (
		shared_ptr<dcp::Subtitle> (
			new dcp::SubtitleImage (
				dcp::ArrayData ("test/data/sub.png"),
				dcp::Time (0, 4,  9, 22, 24),
				dcp::Time (0, 4, 11, 22, 24),
				0,
				dcp::HAlign::CENTER,
				0.8,
				dcp::VAlign::TOP,
				dcp::Time (0, 0, 0, 0, 24),
				dcp::Time (0, 0, 0, 0, 24)
				)
			)
		);

	c->_id = "a6c58cff-3e1e-4b38-acec-a42224475ef6";
	boost::filesystem::remove_all ("build/test/write_interop_subtitle_test3");
	boost::filesystem::create_directories ("build/test/write_interop_subtitle_test3");
	c->write ("build/test/write_interop_subtitle_test3/subs.xml");

	shared_ptr<dcp::Reel> reel (new dcp::Reel());
	reel->add(shared_ptr<dcp::ReelSubtitleAsset>(new dcp::ReelSubtitleAsset(c, dcp::Fraction(24, 1), 6046, 0)));

	string const issue_date = "2018-09-02T04:45:18+00:00";
	string const issuer = "libdcp";
	string const creator = "libdcp";
	string const annotation_text = "Created by libdcp";

	auto cpl = make_shared<dcp::CPL>("My film", dcp::ContentKind::FEATURE);
	cpl->add (reel);
	cpl->set_issuer (issuer);
	cpl->set_creator (creator);
	cpl->set_issue_date (issue_date);
	cpl->set_annotation_text (annotation_text);
	auto cv = cpl->content_version();
	BOOST_REQUIRE (cv);
	cv->label_text = "foo";
	cpl->set_content_version (*cv);

	dcp::DCP dcp ("build/test/write_interop_subtitle_test3");
	dcp.add (cpl);
	dcp.write_xml (dcp::Standard::INTEROP, issuer, creator, issue_date, annotation_text);

	check_xml (
		dcp::file_to_string("test/ref/write_interop_subtitle_test3/subs.xml"),
		dcp::file_to_string("build/test/write_interop_subtitle_test3/subs.xml"),
		vector<string>()
		);
	check_file ("build/test/write_interop_subtitle_test3/d36f4bb3-c4fa-4a95-9915-6fec3110cd71.png", "test/data/sub.png");

	check_xml (
		dcp::file_to_string("test/ref/write_interop_subtitle_test3/ASSETMAP"),
		dcp::file_to_string("build/test/write_interop_subtitle_test3/ASSETMAP"),
		vector<string>()
		);

	check_xml (
		dcp::file_to_string("test/ref/write_interop_subtitle_test3/pkl.xml"),
		dcp::file_to_string("build/test/write_interop_subtitle_test3/pkl_6a9e31a6-50a4-4ecb-8683-fa667848470a.xml"),
		vector<string>()
		);
}

/* Write some subtitle content as SMPTE XML and check that it is right */
BOOST_AUTO_TEST_CASE (write_smpte_subtitle_test)
{
	dcp::SMPTESubtitleAsset c;
	c.set_reel_number (1);
	c.set_language (dcp::LanguageTag("en"));
	c.set_content_title_text ("Test");
	c.set_issue_date (dcp::LocalTime ("2016-04-01T03:52:00+00:00"));

	c.add (
		make_shared<dcp::SubtitleString> (
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
			dcp::HAlign::CENTER,
			0.8,
			dcp::VAlign::TOP,
			dcp::Direction::LTR,
			"Hello world",
			dcp::Effect::NONE,
			dcp::Colour (0, 0, 0),
			dcp::Time (0, 0, 0, 0, 24),
			dcp::Time (0, 0, 0, 0, 24)
			)
		);

	c.add (
		make_shared<dcp::SubtitleString>(
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
			dcp::HAlign::CENTER,
			0.4,
			dcp::VAlign::BOTTOM,
			dcp::Direction::RTL,
			"What's going on",
			dcp::Effect::BORDER,
			dcp::Colour (1, 2, 3),
			dcp::Time (1, 2, 3, 4, 24),
			dcp::Time (5, 6, 7, 8, 24)
			)
		);

	c._xml_id = "a6c58cff-3e1e-4b38-acec-a42224475ef6";

	check_xml (
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<dcst:SubtitleReel xmlns:dcst=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\" xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
		  "<dcst:Id>urn:uuid:a6c58cff-3e1e-4b38-acec-a42224475ef6</dcst:Id>"
		  "<dcst:ContentTitleText>Test</dcst:ContentTitleText>"
		  "<dcst:IssueDate>2016-04-01T03:52:00.000+00:00</dcst:IssueDate>"
		  "<dcst:ReelNumber>1</dcst:ReelNumber>"
		  "<dcst:Language>en</dcst:Language>"
		  "<dcst:EditRate>24 1</dcst:EditRate>"
		  "<dcst:TimeCodeRate>24</dcst:TimeCodeRate>"
		  "<dcst:SubtitleList>"
		    "<dcst:Font AspectAdjust=\"1.0\" Color=\"FFFFFFFF\" Effect=\"none\" EffectColor=\"FF000000\" ID=\"Frutiger\" Italic=\"no\" Script=\"normal\" Size=\"48\" Underline=\"no\" Weight=\"normal\">"
		      "<dcst:Subtitle SpotNumber=\"1\" TimeIn=\"00:04:09:22\" TimeOut=\"00:04:11:22\" FadeUpTime=\"00:00:00:00\" FadeDownTime=\"00:00:00:00\">"
		        "<dcst:Text Valign=\"top\" Vposition=\"80\">Hello world</dcst:Text>"
		      "</dcst:Subtitle>"
		    "</dcst:Font>"
		    "<dcst:Font AspectAdjust=\"1.0\" Color=\"FF800040\" Effect=\"border\" EffectColor=\"FF010203\" Italic=\"yes\" Script=\"normal\" Size=\"91\" Underline=\"yes\" Weight=\"bold\">"
		      "<dcst:Subtitle SpotNumber=\"2\" TimeIn=\"05:41:00:21\" TimeOut=\"06:12:15:21\" FadeUpTime=\"01:02:03:04\" FadeDownTime=\"05:06:07:08\">"
		        "<dcst:Text Valign=\"bottom\" Vposition=\"40\" Direction=\"rtl\">What's going on</dcst:Text>"
		      "</dcst:Subtitle>"
		    "</dcst:Font>"
		  "</dcst:SubtitleList>"
		"</dcst:SubtitleReel>",
		c.xml_as_string (),
		vector<string>()
		);
}

/* Write some subtitle content as SMPTE XML and check that it is right.
   This includes in-line font changes.
*/
BOOST_AUTO_TEST_CASE (write_smpte_subtitle_test2)
{
	dcp::SMPTESubtitleAsset c;
	c.set_reel_number (1);
	c.set_language (dcp::LanguageTag("en"));
	c.set_content_title_text ("Test");
	c.set_issue_date (dcp::LocalTime ("2016-04-01T03:52:00+00:00"));

	c.add (
		make_shared<dcp::SubtitleString>(
			string ("Arial"),
			false,
			false,
			false,
			dcp::Colour (255, 255, 255),
			48,
			1.0,
			dcp::Time (0, 0, 1, 0, 24),
			dcp::Time (0, 0, 9, 0, 24),
			0,
			dcp::HAlign::CENTER,
			0.8,
			dcp::VAlign::TOP,
			dcp::Direction::LTR,
			"Testing is ",
			dcp::Effect::NONE,
			dcp::Colour (0, 0, 0),
			dcp::Time (0, 0, 0, 0, 24),
			dcp::Time (0, 0, 0, 0, 24)
			)
		);

	c.add (
		make_shared<dcp::SubtitleString>(
			string ("Arial"),
			true,
			false,
			false,
			dcp::Colour (255, 255, 255),
			48,
			1.0,
			dcp::Time (0, 0, 1, 0, 24),
			dcp::Time (0, 0, 9, 0, 24),
			0,
			dcp::HAlign::CENTER,
			0.8,
			dcp::VAlign::TOP,
			dcp::Direction::LTR,
			"really",
			dcp::Effect::NONE,
			dcp::Colour (0, 0, 0),
			dcp::Time (0, 0, 0, 0, 24),
			dcp::Time (0, 0, 0, 0, 24)
			)
		);

	c.add (
		make_shared<dcp::SubtitleString>(
			string ("Arial"),
			false,
			false,
			false,
			dcp::Colour (255, 255, 255),
			48,
			1.0,
			dcp::Time (0, 0, 1, 0, 24),
			dcp::Time (0, 0, 9, 0, 24),
			0,
			dcp::HAlign::CENTER,
			0.8,
			dcp::VAlign::TOP,
			dcp::Direction::LTR,
			" fun",
			dcp::Effect::NONE,
			dcp::Colour (0, 0, 0),
			dcp::Time (0, 0, 0, 0, 24),
			dcp::Time (0, 0, 0, 0, 24)
			)
		);

	c.add (
		make_shared<dcp::SubtitleString>(
			string ("Arial"),
			false,
			false,
			false,
			dcp::Colour (255, 255, 255),
			48,
			1.0,
			dcp::Time (0, 0, 1, 0, 24),
			dcp::Time (0, 0, 9, 0, 24),
			0,
			dcp::HAlign::CENTER,
			0.9,
			dcp::VAlign::TOP,
			dcp::Direction::LTR,
			"This is the ",
			dcp::Effect::NONE,
			dcp::Colour (0, 0, 0),
			dcp::Time (0, 0, 0, 0, 24),
			dcp::Time (0, 0, 0, 0, 24)
			)
		);

	c.add (
		make_shared<dcp::SubtitleString>(
			string ("Arial"),
			true,
			false,
			false,
			dcp::Colour (255, 255, 255),
			48,
			1.0,
			dcp::Time (0, 0, 1, 0, 24),
			dcp::Time (0, 0, 9, 0, 24),
			0,
			dcp::HAlign::CENTER,
			0.9,
			dcp::VAlign::TOP,
			dcp::Direction::LTR,
			"second",
			dcp::Effect::NONE,
			dcp::Colour (0, 0, 0),
			dcp::Time (0, 0, 0, 0, 24),
			dcp::Time (0, 0, 0, 0, 24)
			)
		);

	c.add (
		make_shared<dcp::SubtitleString>(
			string ("Arial"),
			false,
			false,
			false,
			dcp::Colour (255, 255, 255),
			48,
			1.0,
			dcp::Time (0, 0, 1, 0, 24),
			dcp::Time (0, 0, 9, 0, 24),
			0,
			dcp::HAlign::CENTER,
			0.9,
			dcp::VAlign::TOP,
			dcp::Direction::LTR,
			" line",
			dcp::Effect::NONE,
			dcp::Colour (0, 0, 0),
			dcp::Time (0, 0, 0, 0, 24),
			dcp::Time (0, 0, 0, 0, 24)
			)
		);

	c._xml_id = "a6c58cff-3e1e-4b38-acec-a42224475ef6";

	check_xml (
		c.xml_as_string(),
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<dcst:SubtitleReel xmlns:dcst=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\" xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
		  "<dcst:Id>urn:uuid:a6c58cff-3e1e-4b38-acec-a42224475ef6</dcst:Id>"
		  "<dcst:ContentTitleText>Test</dcst:ContentTitleText>"
		  "<dcst:IssueDate>2016-04-01T03:52:00.000+00:00</dcst:IssueDate>"
		  "<dcst:ReelNumber>1</dcst:ReelNumber>"
		  "<dcst:Language>en</dcst:Language>"
		  "<dcst:EditRate>24 1</dcst:EditRate>"
		  "<dcst:TimeCodeRate>24</dcst:TimeCodeRate>"
		  "<dcst:SubtitleList>"
		    "<dcst:Font AspectAdjust=\"1.0\" Color=\"FFFFFFFF\" Effect=\"none\" EffectColor=\"FF000000\" ID=\"Arial\" Script=\"normal\" Size=\"48\" Underline=\"no\" Weight=\"normal\">"
		      "<dcst:Subtitle SpotNumber=\"1\" TimeIn=\"00:00:01:00\" TimeOut=\"00:00:09:00\" FadeUpTime=\"00:00:00:00\" FadeDownTime=\"00:00:00:00\">"
		        "<dcst:Text Valign=\"top\" Vposition=\"80\">"
		          "<dcst:Font Italic=\"no\">Testing is </dcst:Font>"
		          "<dcst:Font Italic=\"yes\">really</dcst:Font>"
		          "<dcst:Font Italic=\"no\"> fun</dcst:Font>"
		        "</dcst:Text>"
		        "<dcst:Text Valign=\"top\" Vposition=\"90\">"
		          "<dcst:Font Italic=\"no\">This is the </dcst:Font>"
		          "<dcst:Font Italic=\"yes\">second</dcst:Font>"
		          "<dcst:Font Italic=\"no\"> line</dcst:Font>"
		        "</dcst:Text>"
		      "</dcst:Subtitle>"
		    "</dcst:Font>"
		  "</dcst:SubtitleList>"
		"</dcst:SubtitleReel>",
		vector<string>()
		);
}

/* Write some subtitle content as SMPTE using bitmaps and check that it is right */
BOOST_AUTO_TEST_CASE (write_smpte_subtitle_test3)
{
	dcp::SMPTESubtitleAsset c;
	c.set_reel_number (1);
	c.set_language (dcp::LanguageTag("en"));
	c.set_content_title_text ("Test");

	c.add (
		make_shared<dcp::SubtitleImage>(
			dcp::ArrayData ("test/data/sub.png"),
			dcp::Time (0, 4,  9, 22, 24),
			dcp::Time (0, 4, 11, 22, 24),
			0,
			dcp::HAlign::CENTER,
			0.8,
			dcp::VAlign::TOP,
			dcp::Time (0, 0, 0, 0, 24),
			dcp::Time (0, 0, 0, 0, 24)
			)
	      );

	c._id = "a6c58cff-3e1e-4b38-acec-a42224475ef6";

	boost::filesystem::create_directories ("build/test/write_smpte_subtitle_test3");
	c.write ("build/test/write_smpte_subtitle_test3/subs.mxf");

	/* XXX: check this result when we can read them back in again */
}
