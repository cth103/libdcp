/*
    Copyright (C) 2015-2016 Carl Hetherington <cth@carlh.net>

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
#include "test.h"
#include <boost/test/unit_test.hpp>

using std::list;
using std::string;
using boost::shared_ptr;

/* Write some subtitle content as Interop XML and check that it is right */
BOOST_AUTO_TEST_CASE (write_interop_subtitle_test)
{
	dcp::InteropSubtitleAsset c;
	c.set_reel_number ("1");
	c.set_language ("EN");
	c.set_movie_title ("Test");

	c.add (
		dcp::SubtitleString (
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
		);

	c.add (
		dcp::SubtitleString (
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
		);

	c._id = "a6c58cff-3e1e-4b38-acec-a42224475ef6";

	check_xml (
		c.xml_as_string (),
		"<DCSubtitle Version=\"1.0\">\n"
		"  <SubtitleID>a6c58cff-3e1e-4b38-acec-a42224475ef6</SubtitleID>\n"
		"  <MovieTitle>Test</MovieTitle>\n"
		"  <ReelNumber>1</ReelNumber>\n"
		"  <Language>EN</Language>\n"
		"  <Font Id=\"Frutiger\" Italic=\"no\" Color=\"FFFFFFFF\" Size=\"48\" Effect=\"none\" EffectColor=\"FF000000\" Script=\"normal\" Underlined=\"no\" Weight=\"normal\">\n"
		"    <Subtitle SpotNumber=\"1\" TimeIn=\"00:04:09:229\" TimeOut=\"00:04:11:229\" FadeUpTime=\"0\" FadeDownTime=\"0\">\n"
		"      <Text VAlign=\"top\" VPosition=\"80\">Hello world</Text>\n"
		"    </Subtitle>\n"
		"  </Font>\n"
		"  <Font Italic=\"yes\" Color=\"FF800040\" Size=\"91\" Effect=\"border\" EffectColor=\"FF010203\" Script=\"normal\" Underlined=\"yes\" Weight=\"bold\">\n"
		"    <Subtitle SpotNumber=\"2\" TimeIn=\"05:41:00:218\" TimeOut=\"06:12:15:218\" FadeUpTime=\"930792\" FadeDownTime=\"4591834\">\n"
		"      <Text VAlign=\"bottom\" VPosition=\"40\">What's going on</Text>\n"
		"    </Subtitle>\n"
		"  </Font>\n"
		"</DCSubtitle>",
		list<string> ()
		);
}

/* Write some subtitle content as SMPTE XML and check that it is right */
BOOST_AUTO_TEST_CASE (write_smpte_subtitle_test)
{
	dcp::SMPTESubtitleAsset c;
	c.set_reel_number (1);
	c.set_language ("EN");
	c.set_content_title_text ("Test");
	c.set_issue_date (dcp::LocalTime ("2016-04-01T03:52:00+00:00"));

	c.add (
		dcp::SubtitleString (
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
		);

	c.add (
		dcp::SubtitleString (
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
		);

	c._id = "a6c58cff-3e1e-4b38-acec-a42224475ef6";

	check_xml (
		c.xml_as_string (),
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<dcst:SubtitleReel xmlns:dcst=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\" xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">\n"
		"  <dcst:Id>urn:uuid:a6c58cff-3e1e-4b38-acec-a42224475ef6</dcst:Id>\n"
		"  <dcst:ContentTitleText>Test</dcst:ContentTitleText>\n"
		"  <dcst:IssueDate>2016-04-01T03:52:00.000+00:00</dcst:IssueDate>\n"
		"  <dcst:ReelNumber>1</dcst:ReelNumber>\n"
		"  <dcst:Language>EN</dcst:Language>\n"
		"  <dcst:EditRate>24 1</dcst:EditRate>\n"
		"  <dcst:TimeCodeRate>24</dcst:TimeCodeRate>\n"
		"  <dcst:SubtitleList>\n"
		"    <dcst:Font ID=\"Frutiger\" Italic=\"no\" Color=\"FFFFFFFF\" Size=\"48\" Effect=\"none\" EffectColor=\"FF000000\" Script=\"normal\" Underline=\"no\" Weight=\"normal\">\n"
		"      <dcst:Subtitle SpotNumber=\"1\" TimeIn=\"00:04:09:22\" TimeOut=\"00:04:11:22\" FadeUpTime=\"00:00:00:00\" FadeDownTime=\"00:00:00:00\">\n"
		"        <dcst:Text Valign=\"top\" Vposition=\"80\">Hello world</dcst:Text>\n"
		"      </dcst:Subtitle>\n"
		"    </dcst:Font>\n"
		"    <dcst:Font Italic=\"yes\" Color=\"FF800040\" Size=\"91\" Effect=\"border\" EffectColor=\"FF010203\" Script=\"normal\" Underline=\"yes\" Weight=\"bold\">\n"
		"      <dcst:Subtitle SpotNumber=\"2\" TimeIn=\"05:41:00:21\" TimeOut=\"06:12:15:21\" FadeUpTime=\"01:02:03:04\" FadeDownTime=\"05:06:07:08\">\n"
		"        <dcst:Text Valign=\"bottom\" Vposition=\"40\" Direction=\"rtl\">What's going on</dcst:Text>\n"
		"      </dcst:Subtitle>\n"
		"    </dcst:Font>\n"
		"  </dcst:SubtitleList>\n"
		"</dcst:SubtitleReel>\n",
		list<string> ()
		);
}
