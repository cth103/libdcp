/*
    Copyright (C) 2015 Carl Hetherington <cth@carlh.net>

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

#include "interop_subtitle_content.h"
#include "subtitle_string.h"
#include "test.h"
#include <boost/test/unit_test.hpp>

using std::list;
using std::string;
using boost::shared_ptr;

/* Write some subtitle content as Interop XML and check that it is right */
BOOST_AUTO_TEST_CASE (write_subtitle_test)
{
	dcp::InteropSubtitleContent c ("Test", "EN");

	c.add (
		dcp::SubtitleString (
			string ("Frutiger"),
			false,
			dcp::Colour (255, 255, 255),
			48,
			dcp::Time (0, 4,  9, 22, 24),
			dcp::Time (0, 4, 11, 22, 24),
			0.8,
			dcp::TOP,
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
			dcp::Colour (128, 0, 64),
			91,
			dcp::Time (5, 41,  0, 21, 24),
			dcp::Time (6, 12, 15, 21, 24),
			0.4,
			dcp::BOTTOM,
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
		"    <Subtitle SpotNumber=\"1\" TimeIn=\"00:04:09:022\" TimeOut=\"00:04:11:022\" FadeUpTime=\"0\" FadeDownTime=\"0\">\n"
		"      <Text VAlign=\"top\" VPosition=\"80\">Hello world</Text>\n"
		"    </Subtitle>\n"
		"  </Font>\n"
		"  <Font Italic=\"yes\" Color=\"FF800040\" Size=\"91\" Effect=\"border\" EffectColor=\"FF010203\" Script=\"normal\" Underlined=\"no\" Weight=\"normal\">\n"
		"    <Subtitle SpotNumber=\"2\" TimeIn=\"05:41:00:021\" TimeOut=\"06:12:15:021\" FadeUpTime=\"930790\" FadeDownTime=\"4591830\">\n"
		"      <Text VAlign=\"bottom\" VPosition=\"40\">What's going on</Text>\n"
		"    </Subtitle>\n"
		"  </Font>\n"
		"</DCSubtitle>",
		list<string> ()
		);
}

