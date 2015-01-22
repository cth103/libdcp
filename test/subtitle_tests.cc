/*
    Copyright (C) 2012-2013 Carl Hetherington <cth@carlh.net>

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

#include <boost/test/unit_test.hpp>
#include "subtitle_asset.h"

using std::list;
using boost::shared_ptr;

/* Load a subtitle asset from XML and check that it is read correctly */
BOOST_AUTO_TEST_CASE (subtitles1)
{
	libdcp::SubtitleAsset subs ("test/data", "subs1.xml");

	BOOST_CHECK_EQUAL (subs.language(), "French");

	list<shared_ptr<libdcp::Subtitle> > s = subs.subtitles_during (libdcp::Time (0, 0, 6, 1, 250), libdcp::Time (0, 0, 6, 2, 250));
	BOOST_CHECK_EQUAL (s.size(), 1);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   39,
				   libdcp::Time (0, 0, 5, 198, 250),
				   libdcp::Time (0, 0, 7, 115, 250),
				   15,
				   libdcp::VERTICAL_BOTTOM,
				   libdcp::HORIZONTAL_CENTER,
				   "My jacket was Idi Amin's",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 1, 250),
				   libdcp::Time (0, 0, 0, 1, 250)
				   ));
							 
	s = subs.subtitles_during (libdcp::Time (0, 0, 7, 190, 250), libdcp::Time (0, 0, 7, 191, 250));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   39,
				   libdcp::Time (0, 0, 7, 177, 250),
				   libdcp::Time (0, 0, 11, 31, 250),
				   21,
				   libdcp::VERTICAL_BOTTOM,
				   libdcp::HORIZONTAL_CENTER,
				   "My corset was H.M. The Queen's",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 1, 250),
				   libdcp::Time (0, 0, 0, 1, 250)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   39,
				   libdcp::Time (0, 0, 7, 177, 250),
				   libdcp::Time (0, 0, 11, 31, 250),
				   15,
				   libdcp::VERTICAL_BOTTOM,
				   libdcp::HORIZONTAL_CENTER,
				   "My large wonderbra",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 1, 250),
				   libdcp::Time (0, 0, 0, 1, 250)
				   ));

	s = subs.subtitles_during (libdcp::Time (0, 0, 11, 95, 250), libdcp::Time (0, 0, 11, 96, 250));
	BOOST_CHECK_EQUAL (s.size(), 1);
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   39,
				   libdcp::Time (0, 0, 11, 94, 250),
				   libdcp::Time (0, 0, 13, 63, 250),
				   15,
				   libdcp::VERTICAL_BOTTOM,
				   libdcp::HORIZONTAL_CENTER,
				   "Once belonged to the Shah",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 1, 250),
				   libdcp::Time (0, 0, 0, 1, 250)
				   ));

	s = subs.subtitles_during (libdcp::Time (0, 0, 14, 42, 250), libdcp::Time (0, 0, 14, 43, 250));
	BOOST_CHECK_EQUAL (s.size(), 1);
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   39,
				   libdcp::Time (0, 0, 13, 104, 250),
				   libdcp::Time (0, 0, 15, 177, 250),
				   15,
				   libdcp::VERTICAL_BOTTOM,
				   libdcp::HORIZONTAL_CENTER,
				   "And these are Roy Hattersley's jeans",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 1, 250),
				   libdcp::Time (0, 0, 0, 1, 250)
				   ));
}

/** And similarly for another one */
BOOST_AUTO_TEST_CASE (subtitles2)
{
	libdcp::SubtitleAsset subs ("test/data", "subs2.xml");

	list<shared_ptr<libdcp::Subtitle> > s = subs.subtitles_during (libdcp::Time (0, 0, 42, 100, 250), libdcp::Time (0, 0, 42, 101, 250));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 0, 41, 62, 250),
				   libdcp::Time (0, 0, 43, 52, 250),
				   89,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "At afternoon tea with John Peel",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 0, 41, 62, 250),
				   libdcp::Time (0, 0, 43, 52, 250),
				   95,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "I enquired if his accent was real",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (libdcp::Time (0, 0, 50, 50, 250), libdcp::Time (0, 0, 50, 51, 250));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 0, 50, 42, 250),
				   libdcp::Time (0, 0, 52, 21, 250),
				   89,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "He said \"out of the house",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 0, 50, 42, 250),
				   libdcp::Time (0, 0, 52, 21, 250),
				   95,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "I'm incredibly scouse",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (libdcp::Time (0, 1, 2, 300, 250), libdcp::Time (0, 1, 2, 301, 250));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 2, 208, 250),
				   libdcp::Time (0, 1, 4, 10, 250),
				   89,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "At home it depends how I feel.\"",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 2, 208, 250),
				   libdcp::Time (0, 1, 4, 10, 250),
				   95,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "I spent a long weekend in Brighton",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (libdcp::Time (0, 1, 15, 50, 250), libdcp::Time (0, 1, 15, 51, 250));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 15, 42, 250),
				   libdcp::Time (0, 1, 16, 42, 250),
				   89,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "With the legendary Miss Enid Blyton",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 15, 42, 250),
				   libdcp::Time (0, 1, 16, 42, 250),
				   95,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "She said \"you be Noddy",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (libdcp::Time (0, 1, 27, 200, 250), libdcp::Time (0, 1, 27, 201, 250));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 27, 115, 250),
				   libdcp::Time (0, 1, 28, 208, 250),
				   89,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "That curious creature the Sphinx",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 27, 115, 250),
				   libdcp::Time (0, 1, 28, 208, 250),
				   95,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "Is smarter than anyone thinks",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (libdcp::Time (0, 1, 42, 300, 250), libdcp::Time (0, 1, 42, 301, 250));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 42, 229, 250),
				   libdcp::Time (0, 1, 45, 62, 250),
				   89,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "It sits there and smirks",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 42, 229, 250),
				   libdcp::Time (0, 1, 45, 62, 250),
				   95,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "And you don't think it works",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (libdcp::Time (0, 1, 45, 200, 250), libdcp::Time (0, 1, 45, 201, 250));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 45, 146, 250),
				   libdcp::Time (0, 1, 47, 94, 250),
				   89,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "Then when you're not looking, it winks.",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 45, 146, 250),
				   libdcp::Time (0, 1, 47, 94, 250),
				   95,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "When it snows you will find Sister Sledge",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (libdcp::Time (0, 1, 47, 249, 250), libdcp::Time (0, 1, 47, 250, 250));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 47, 146, 250),
				   libdcp::Time (0, 1, 48, 167, 250),
				   89,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "Out mooning, at night, on the ledge",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 47, 146, 250),
				   libdcp::Time (0, 1, 48, 167, 250),
				   95,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "One storey down",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (libdcp::Time (0, 2, 6, 210, 250), libdcp::Time (0, 2, 6, 211, 250));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 2, 5, 208, 250),
				   libdcp::Time (0, 2, 7, 31, 250),
				   89,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "HELLO",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 2, 5, 208, 250),
				   libdcp::Time (0, 2, 7, 31, 250),
				   95,
				   libdcp::VERTICAL_TOP,
				   libdcp::HORIZONTAL_CENTER,
				   "WORLD",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 250),
				   libdcp::Time (0, 0, 0, 0, 250)
				   ));
}

/* A very simple SMPTE one */
BOOST_AUTO_TEST_CASE (subtitles3)
{
	libdcp::SubtitleAsset subs ("test/data", "subs3.xml");

	list<shared_ptr<libdcp::Subtitle> > s = subs.subtitles_during (libdcp::Time (0, 0, 0, 0, 25), libdcp::Time (0, 0, 7, 0, 25));

	BOOST_REQUIRE_EQUAL (s.size(), 1);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 0, 4, 21, 25),
				   libdcp::Time (0, 0, 6, 5, 25),
				   8,
				   libdcp::VERTICAL_BOTTOM,
				   libdcp::HORIZONTAL_CENTER,
				   "Hello world",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 25),
				   libdcp::Time (0, 0, 0, 0, 25)
				   ));
}

/* <Font italic="yes"> in the middle of a string */
BOOST_AUTO_TEST_CASE (subtitles4)
{
	libdcp::SubtitleAsset subs ("test/data", "subs4.xml");

	list<shared_ptr<libdcp::Subtitle> > s = subs.subtitles_during (libdcp::Time (0, 0, 0, 0, 25), libdcp::Time (0, 0, 7, 0, 25));

	BOOST_REQUIRE_EQUAL (s.size(), 1);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 0, 4, 21, 25),
				   libdcp::Time (0, 0, 6, 5, 25),
				   8,
				   libdcp::VERTICAL_BOTTOM,
				   libdcp::HORIZONTAL_CENTER,
				   "Hello <i>there</i> world",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0, 25),
				   libdcp::Time (0, 0, 0, 0, 25)
				   ));
}
