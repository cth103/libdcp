/*
    Copyright (C) 2018 Carl Hetherington <cth@carlh.net>

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


#include "smpte_load_font_node.h"
#include "smpte_subtitle_asset.h"
#include "subtitle_image.h"
#include "test.h"
#include "types.h"
#include <boost/optional/optional_io.hpp>
#include <boost/test/unit_test.hpp>


using std::list;
using std::string;
using std::vector;
using boost::dynamic_pointer_cast;
using boost::optional;
using boost::shared_ptr;


BOOST_AUTO_TEST_CASE (smpte_subtitle_id_test)
{
	dcp::SMPTESubtitleAsset subs;
	subs.add(
		shared_ptr<dcp::Subtitle> (
			new dcp::SubtitleString(
				optional<string>(),
				false, false, false,
				dcp::Colour(),
				64,
				1,
				dcp::Time(0, 1, 2, 3, 24),
				dcp::Time(0, 2, 2, 3, 24),
				0.5,
				dcp::HALIGN_CENTER,
				0.5,
				dcp::VALIGN_CENTER,
				dcp::DIRECTION_LTR,
				"Hello",
				dcp::NONE,
				dcp::Colour(),
				dcp::Time(0, 0, 0, 0, 24),
				dcp::Time(0, 0, 0, 0, 24)
				)
			)
		);
	subs.write("build/test/smpte_subtitle_id_test.mxf");

	dcp::SMPTESubtitleAsset check("build/test/smpte_subtitle_id_test.mxf");
	BOOST_CHECK(check.id() != check.xml_id());
}


/** Check reading of a SMPTE subtitle file */
BOOST_AUTO_TEST_CASE (read_smpte_subtitle_test)
{
	dcp::SMPTESubtitleAsset sc (
		private_test /
		"data" /
		"JourneyToJah_TLR-1_F_EN-DE-FR_CH_51_2K_LOK_20140225_DGL_SMPTE_OV" /
		"8b48f6ae-c74b-4b80-b994-a8236bbbad74_sub.mxf"
		);

	BOOST_CHECK_EQUAL (sc.id(), "8b48f6ae-c74b-4b80-b994-a8236bbbad74");
	BOOST_CHECK_EQUAL (sc.content_title_text(), "Journey to Jah");
	BOOST_REQUIRE (sc.annotation_text());
	BOOST_CHECK_EQUAL (sc.annotation_text().get(), "Journey to Jah");
	BOOST_CHECK_EQUAL (sc.issue_date(), dcp::LocalTime ("2014-02-25T11:22:48.000-00:00"));
	BOOST_REQUIRE (sc.reel_number());
	BOOST_CHECK_EQUAL (sc.reel_number().get(), 1);
	BOOST_REQUIRE (sc.language ());
	BOOST_CHECK_EQUAL (sc.language().get (), "de");
	BOOST_CHECK_EQUAL (sc.edit_rate(), dcp::Fraction (25, 1));
	BOOST_CHECK_EQUAL (sc.time_code_rate(), 25);
	BOOST_CHECK_EQUAL (sc.start_time(), dcp::Time (0, 0, 0, 0, 25));
	auto lfn = sc.load_font_nodes ();
	BOOST_REQUIRE_EQUAL (lfn.size(), 1);
	shared_ptr<dcp::SMPTELoadFontNode> smpte_lfn = dynamic_pointer_cast<dcp::SMPTELoadFontNode> (lfn.front ());
	BOOST_REQUIRE (smpte_lfn);
	BOOST_CHECK_EQUAL (smpte_lfn->id, "theFontId");
	BOOST_CHECK_EQUAL (smpte_lfn->urn, "9118bbce-4105-4a05-b37c-a5a6f75e1fea");
	BOOST_REQUIRE_EQUAL (sc.subtitles().size(), 63);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(sc.subtitles().front()));
	BOOST_CHECK_EQUAL (dynamic_pointer_cast<const dcp::SubtitleString>(sc.subtitles().front())->text(), "Noch mal.");
	BOOST_CHECK_EQUAL (sc.subtitles().front()->in(), dcp::Time (0, 0, 25, 12, 25));
	BOOST_CHECK_EQUAL (sc.subtitles().front()->out(), dcp::Time (0, 0, 26, 4, 25));
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(sc.subtitles().back()));
	BOOST_CHECK_EQUAL (dynamic_pointer_cast<const dcp::SubtitleString>(sc.subtitles().back())->text(), "Prochainement");
	BOOST_CHECK_EQUAL (sc.subtitles().back()->in(), dcp::Time (0, 1, 57, 17, 25));
	BOOST_CHECK_EQUAL (sc.subtitles().back()->out(), dcp::Time (0, 1, 58, 12, 25));
}


/** And another one featuring <Font> within <Text> */
BOOST_AUTO_TEST_CASE (read_smpte_subtitle_test2)
{
	dcp::SMPTESubtitleAsset sc (private_test / "olsson.xml");

	auto subs = sc.subtitles();
	BOOST_REQUIRE_EQUAL (subs.size(), 6);
	auto i = subs.begin();
	auto is = dynamic_pointer_cast<const dcp::SubtitleString>(*i);
	BOOST_REQUIRE (is);
	BOOST_CHECK_EQUAL (is->text(), "Testing is ");
	BOOST_CHECK (!is->italic());
	++i;
	is = dynamic_pointer_cast<const dcp::SubtitleString>(*i);
	BOOST_REQUIRE (is);
	BOOST_CHECK_EQUAL (is->text(), "really");
	BOOST_CHECK (is->italic());
	++i;
	is = dynamic_pointer_cast<const dcp::SubtitleString>(*i);
	BOOST_REQUIRE (is);
	BOOST_CHECK_EQUAL (is->text(), " fun!");
	BOOST_CHECK (!is->italic());
	++i;
	is = dynamic_pointer_cast<const dcp::SubtitleString>(*i);
	BOOST_REQUIRE (is);
	BOOST_CHECK_EQUAL (is->text(), "This is the ");
	BOOST_CHECK (!is->italic());
	++i;
	is = dynamic_pointer_cast<const dcp::SubtitleString>(*i);
	BOOST_REQUIRE (is);
	BOOST_CHECK_EQUAL (is->text(), "second");
	BOOST_CHECK (is->italic());
	++i;
	is = dynamic_pointer_cast<const dcp::SubtitleString>(*i);
	BOOST_REQUIRE (is);
	BOOST_CHECK_EQUAL (is->text(), " line!");
	BOOST_CHECK (!is->italic());
}


/** And another one featuring image subtitles */
BOOST_AUTO_TEST_CASE (read_smpte_subtitle_test3)
{
	dcp::SMPTESubtitleAsset subs ("test/data/subs.mxf");

	BOOST_REQUIRE_EQUAL (subs.subtitles().size(), 1);
	auto si = dynamic_pointer_cast<const dcp::SubtitleImage>(subs.subtitles().front());
	BOOST_REQUIRE (si);
	BOOST_CHECK (si->png_image() == dcp::Data("test/data/sub.png"));
}


/* Write some subtitle content as SMPTE XML and check that it is right */
BOOST_AUTO_TEST_CASE (write_smpte_subtitle_test)
{
	dcp::SMPTESubtitleAsset c;
	c.set_reel_number (1);
	c.set_language ("en");
	c.set_content_title_text ("Test");
	c.set_issue_date (dcp::LocalTime ("2016-04-01T03:52:00+00:00"));

	c.add (
		shared_ptr<dcp::SubtitleString> (
			new dcp::SubtitleString(
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
		shared_ptr<dcp::SubtitleString>(
			new dcp::SubtitleString(
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
				dcp::DIRECTION_RTL,
				"What's going on",
				dcp::BORDER,
				dcp::Colour (1, 2, 3),
				dcp::Time (1, 2, 3, 4, 24),
				dcp::Time (5, 6, 7, 8, 24)
				)
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
		list<string>()
		);
}

/* Write some subtitle content as SMPTE XML and check that it is right.
   This includes in-line font changes.
*/
BOOST_AUTO_TEST_CASE (write_smpte_subtitle_test2)
{
	dcp::SMPTESubtitleAsset c;
	c.set_reel_number (1);
	c.set_language ("en");
	c.set_content_title_text ("Test");
	c.set_issue_date (dcp::LocalTime ("2016-04-01T03:52:00+00:00"));

	c.add (
		shared_ptr<dcp::SubtitleString>(
			new dcp::SubtitleString(
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
				dcp::HALIGN_CENTER,
				0.8,
				dcp::VALIGN_TOP,
				dcp::DIRECTION_LTR,
				"Testing is ",
				dcp::NONE,
				dcp::Colour (0, 0, 0),
				dcp::Time (0, 0, 0, 0, 24),
				dcp::Time (0, 0, 0, 0, 24)
				)
			)
		);

	c.add (
		shared_ptr<dcp::SubtitleString>(
			new dcp::SubtitleString(
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
				dcp::HALIGN_CENTER,
				0.8,
				dcp::VALIGN_TOP,
				dcp::DIRECTION_LTR,
				"really",
				dcp::NONE,
				dcp::Colour (0, 0, 0),
				dcp::Time (0, 0, 0, 0, 24),
				dcp::Time (0, 0, 0, 0, 24)
				)
			)
		);

	c.add (
		shared_ptr<dcp::SubtitleString>(
			new dcp::SubtitleString(
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
				dcp::HALIGN_CENTER,
				0.8,
				dcp::VALIGN_TOP,
				dcp::DIRECTION_LTR,
				" fun",
				dcp::NONE,
				dcp::Colour (0, 0, 0),
				dcp::Time (0, 0, 0, 0, 24),
				dcp::Time (0, 0, 0, 0, 24)
				)
			)
		);

	c.add (
		shared_ptr<dcp::SubtitleString>(
			new dcp::SubtitleString(
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
				dcp::HALIGN_CENTER,
				0.9,
				dcp::VALIGN_TOP,
				dcp::DIRECTION_LTR,
				"This is the ",
				dcp::NONE,
				dcp::Colour (0, 0, 0),
				dcp::Time (0, 0, 0, 0, 24),
				dcp::Time (0, 0, 0, 0, 24)
				)
			)
		);

	c.add (
		shared_ptr<dcp::SubtitleString>(
			new dcp::SubtitleString(
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
				dcp::HALIGN_CENTER,
				0.9,
				dcp::VALIGN_TOP,
				dcp::DIRECTION_LTR,
				"second",
				dcp::NONE,
				dcp::Colour (0, 0, 0),
				dcp::Time (0, 0, 0, 0, 24),
				dcp::Time (0, 0, 0, 0, 24)
				)
			)
		);

	c.add (
		shared_ptr<dcp::SubtitleString>(
			new dcp::SubtitleString(
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
				dcp::HALIGN_CENTER,
				0.9,
				dcp::VALIGN_TOP,
				dcp::DIRECTION_LTR,
				" line",
				dcp::NONE,
				dcp::Colour (0, 0, 0),
				dcp::Time (0, 0, 0, 0, 24),
				dcp::Time (0, 0, 0, 0, 24)
				)
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
		list<string>()
		);
}

/* Write some subtitle content as SMPTE using bitmaps and check that it is right */
BOOST_AUTO_TEST_CASE (write_smpte_subtitle_test3)
{
	dcp::SMPTESubtitleAsset c;
	c.set_reel_number (1);
	c.set_language ("en");
	c.set_content_title_text ("Test");

	c.add (
		shared_ptr<dcp::SubtitleImage>(
			new dcp::SubtitleImage(
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

	c._id = "a6c58cff-3e1e-4b38-acec-a42224475ef6";

	boost::filesystem::create_directories ("build/test/write_smpte_subtitle_test3");
	c.write ("build/test/write_smpte_subtitle_test3/subs.mxf");

	/* XXX: check this result when we can read them back in again */
}

