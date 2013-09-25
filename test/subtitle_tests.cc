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

	list<shared_ptr<libdcp::Subtitle> > s = subs.subtitles_at (libdcp::Time (0, 0, 6, 1));
	BOOST_CHECK_EQUAL (s.size(), 1);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   39,
				   libdcp::Time (0, 0, 5, 198),
				   libdcp::Time (0, 0, 7, 115),
				   15,
				   libdcp::BOTTOM,
				   "My jacket was Idi Amin's",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 1),
				   libdcp::Time (0, 0, 0, 1)
				   ));
							 
	s = subs.subtitles_at (libdcp::Time (0, 0, 7, 190));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   39,
				   libdcp::Time (0, 0, 7, 177),
				   libdcp::Time (0, 0, 11, 31),
				   21,
				   libdcp::BOTTOM,
				   "My corset was H.M. The Queen's",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 1),
				   libdcp::Time (0, 0, 0, 1)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   39,
				   libdcp::Time (0, 0, 7, 177),
				   libdcp::Time (0, 0, 11, 31),
				   15,
				   libdcp::BOTTOM,
				   "My large wonderbra",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 1),
				   libdcp::Time (0, 0, 0, 1)
				   ));

	s = subs.subtitles_at (libdcp::Time (0, 0, 11, 95));
	BOOST_CHECK_EQUAL (s.size(), 1);
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   39,
				   libdcp::Time (0, 0, 11, 94),
				   libdcp::Time (0, 0, 13, 63),
				   15,
				   libdcp::BOTTOM,
				   "Once belonged to the Shah",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 1),
				   libdcp::Time (0, 0, 0, 1)
				   ));

	s = subs.subtitles_at (libdcp::Time (0, 0, 14, 42));
	BOOST_CHECK_EQUAL (s.size(), 1);
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   39,
				   libdcp::Time (0, 0, 13, 104),
				   libdcp::Time (0, 0, 15, 177),
				   15,
				   libdcp::BOTTOM,
				   "And these are Roy Hattersley's jeans",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 1),
				   libdcp::Time (0, 0, 0, 1)
				   ));
}

/** And similarly for another one */
BOOST_AUTO_TEST_CASE (subtitles2)
{
	libdcp::SubtitleAsset subs ("test/data", "subs2.xml");

	list<shared_ptr<libdcp::Subtitle> > s = subs.subtitles_at (libdcp::Time (0, 0, 42, 100));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 0, 41, 62),
				   libdcp::Time (0, 0, 43, 52),
				   89,
				   libdcp::TOP,
				   "At afternoon tea with John Peel",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 0, 41, 62),
				   libdcp::Time (0, 0, 43, 52),
				   95,
				   libdcp::TOP,
				   "I enquired if his accent was real",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));

	s = subs.subtitles_at (libdcp::Time (0, 0, 50, 50));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 0, 50, 42),
				   libdcp::Time (0, 0, 52, 21),
				   89,
				   libdcp::TOP,
				   "He said \"out of the house",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 0, 50, 42),
				   libdcp::Time (0, 0, 52, 21),
				   95,
				   libdcp::TOP,
				   "I'm incredibly scouse",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));

	s = subs.subtitles_at (libdcp::Time (0, 1, 2, 300));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 2, 208),
				   libdcp::Time (0, 1, 4, 10),
				   89,
				   libdcp::TOP,
				   "At home it depends how I feel.\"",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 2, 208),
				   libdcp::Time (0, 1, 4, 10),
				   95,
				   libdcp::TOP,
				   "I spent a long weekend in Brighton",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));

	s = subs.subtitles_at (libdcp::Time (0, 1, 15, 50));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 15, 42),
				   libdcp::Time (0, 1, 16, 42),
				   89,
				   libdcp::TOP,
				   "With the legendary Miss Enid Blyton",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 15, 42),
				   libdcp::Time (0, 1, 16, 42),
				   95,
				   libdcp::TOP,
				   "She said \"you be Noddy",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));

	s = subs.subtitles_at (libdcp::Time (0, 1, 27, 200));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 27, 115),
				   libdcp::Time (0, 1, 28, 208),
				   89,
				   libdcp::TOP,
				   "That curious creature the Sphinx",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 27, 115),
				   libdcp::Time (0, 1, 28, 208),
				   95,
				   libdcp::TOP,
				   "Is smarter than anyone thinks",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));

	s = subs.subtitles_at (libdcp::Time (0, 1, 42, 300));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 42, 229),
				   libdcp::Time (0, 1, 45, 62),
				   89,
				   libdcp::TOP,
				   "It sits there and smirks",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 42, 229),
				   libdcp::Time (0, 1, 45, 62),
				   95,
				   libdcp::TOP,
				   "And you don't think it works",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));

	s = subs.subtitles_at (libdcp::Time (0, 1, 45, 200));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 45, 146),
				   libdcp::Time (0, 1, 47, 94),
				   89,
				   libdcp::TOP,
				   "Then when you're not looking, it winks.",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 45, 146),
				   libdcp::Time (0, 1, 47, 94),
				   95,
				   libdcp::TOP,
				   "When it snows you will find Sister Sledge",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));

	s = subs.subtitles_at (libdcp::Time (0, 1, 47, 249));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 47, 146),
				   libdcp::Time (0, 1, 48, 167),
				   89,
				   libdcp::TOP,
				   "Out mooning, at night, on the ledge",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   false,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 1, 47, 146),
				   libdcp::Time (0, 1, 48, 167),
				   95,
				   libdcp::TOP,
				   "One storey down",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));

	s = subs.subtitles_at (libdcp::Time (0, 2, 6, 210));
	BOOST_CHECK_EQUAL (s.size(), 2);
	BOOST_CHECK_EQUAL (*(s.front().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 2, 5, 208),
				   libdcp::Time (0, 2, 7, 31),
				   89,
				   libdcp::TOP,
				   "HELLO",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));
	BOOST_CHECK_EQUAL (*(s.back().get()), libdcp::Subtitle (
				   "Arial",
				   true,
				   libdcp::Color (255, 255, 255),
				   42,
				   libdcp::Time (0, 2, 5, 208),
				   libdcp::Time (0, 2, 7, 31),
				   95,
				   libdcp::TOP,
				   "WORLD",
				   libdcp::BORDER,
				   libdcp::Color (0, 0, 0),
				   libdcp::Time (0, 0, 0, 0),
				   libdcp::Time (0, 0, 0, 0)
				   ));

	
	
}
