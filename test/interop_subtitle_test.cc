/*
    Copyright (C) 2012-202]1 Carl Hetherington <cth@carlh.net>

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
#include "interop_load_font_node.h"
#include "reel_interop_subtitle_asset.h"
#include "subtitle_string.h"
#include "subtitle_image.h"
#include "test.h"
#include <boost/test/unit_test.hpp>
#include <iostream>


using std::dynamic_pointer_cast;
using std::list;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::vector;


/** Load some subtitle content from Interop XML and check that it is read correctly */
BOOST_AUTO_TEST_CASE (read_interop_subtitle_test1)
{
	dcp::InteropSubtitleAsset subs ("test/data/subs1.xml");

	BOOST_CHECK_EQUAL (subs.id(), "cab5c268-222b-41d2-88ae-6d6999441b17");
	BOOST_CHECK_EQUAL (subs.movie_title(), "Movie Title");
	BOOST_CHECK_EQUAL (subs.reel_number(), "1");
	BOOST_CHECK_EQUAL (subs.language(), "French");

	auto lfn = subs.load_font_nodes ();
	BOOST_REQUIRE_EQUAL (lfn.size(), 1);
	shared_ptr<dcp::InteropLoadFontNode> interop_lfn = dynamic_pointer_cast<dcp::InteropLoadFontNode> (lfn.front ());
	BOOST_REQUIRE (interop_lfn);
	BOOST_CHECK_EQUAL (interop_lfn->id, "theFontId");
	BOOST_CHECK_EQUAL (interop_lfn->uri, "arial.ttf");

	auto s = subs.subtitles_during (dcp::Time (0, 0, 6, 1, 250), dcp::Time (0, 0, 6, 2, 250), false);
	BOOST_REQUIRE_EQUAL (s.size(), 1);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.front()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.front()), dcp::SubtitleString (
				   string ("theFontId"),
				   false,
				   false,
				   false,
				   dcp::Colour (255, 255, 255),
				   39,
				   1.0,
				   dcp::Time (0, 0, 5, 198, 250),
				   dcp::Time (0, 0, 7, 115, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.15,
				   dcp::VAlign::BOTTOM,
				   dcp::Direction::LTR,
				   "My jacket was Idi Amin's",
				   dcp::Effect::BORDER,
				   dcp::Colour (0, 0, 0),
				   dcp::Time (0, 0, 0, 1, 250),
				   dcp::Time (0, 0, 0, 1, 250)
				   ));

	s = subs.subtitles_during (dcp::Time (0, 0, 7, 190, 250), dcp::Time (0, 0, 7, 191, 250), false);
	BOOST_REQUIRE_EQUAL (s.size(), 2);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.front()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.front()), dcp::SubtitleString (
				   string ("theFontId"),
				   true,
				   false,
				   false,
				   dcp::Colour (255, 255, 255),
				   39,
				   1.0,
				   dcp::Time (0, 0, 7, 177, 250),
				   dcp::Time (0, 0, 11, 31, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.21,
				   dcp::VAlign::BOTTOM,
				   dcp::Direction::LTR,
				   "My corset was H.M. The Queen's",
				   dcp::Effect::BORDER,
				   dcp::Colour (0, 0, 0),
				   dcp::Time (0, 0, 0, 1, 250),
				   dcp::Time (0, 0, 0, 1, 250)
				   ));
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.back()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.back()), dcp::SubtitleString (
				   string ("theFontId"),
				   false,
				   false,
				   false,
				   dcp::Colour (255, 255, 255),
				   39,
				   1.0,
				   dcp::Time (0, 0, 7, 177, 250),
				   dcp::Time (0, 0, 11, 31, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.15,
				   dcp::VAlign::BOTTOM,
				   dcp::Direction::LTR,
				   "My large wonderbra",
				   dcp::Effect::BORDER,
				   dcp::Colour (0, 0, 0),
				   dcp::Time (0, 0, 0, 1, 250),
				   dcp::Time (0, 0, 0, 1, 250)
				   ));

	s = subs.subtitles_during (dcp::Time (0, 0, 11, 95, 250), dcp::Time (0, 0, 11, 96, 250), false);
	BOOST_REQUIRE_EQUAL (s.size(), 1);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.back()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.back()), dcp::SubtitleString (
				   string ("theFontId"),
				   false,
				   false,
				   false,
				   dcp::Colour (255, 255, 255),
				   39,
				   1.0,
				   dcp::Time (0, 0, 11, 94, 250),
				   dcp::Time (0, 0, 13, 63, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.15,
				   dcp::VAlign::BOTTOM,
				   dcp::Direction::LTR,
				   "Once belonged to the Shah",
				   dcp::Effect::BORDER,
				   dcp::Colour (0, 0, 0),
				   dcp::Time (0, 0, 0, 1, 250),
				   dcp::Time (0, 0, 0, 1, 250)
				   ));

	s = subs.subtitles_during (dcp::Time (0, 0, 14, 42, 250), dcp::Time (0, 0, 14, 43, 250), false);
	BOOST_REQUIRE_EQUAL (s.size(), 1);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.back()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.back()), dcp::SubtitleString (
				   string ("theFontId"),
				   false,
				   true,
				   true,
				   dcp::Colour (255, 255, 255),
				   39,
				   1.0,
				   dcp::Time (0, 0, 13, 104, 250),
				   dcp::Time (0, 0, 15, 177, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.15,
				   dcp::VAlign::BOTTOM,
				   dcp::Direction::LTR,
				   "And these are Roy Hattersley's jeans",
				   dcp::Effect::BORDER,
				   dcp::Colour (0, 0, 0),
				   dcp::Time (0, 0, 0, 1, 250),
				   dcp::Time (0, 0, 0, 1, 250)
				   ));
}

/** And similarly for another one */
BOOST_AUTO_TEST_CASE (read_interop_subtitle_test2)
{
	dcp::InteropSubtitleAsset subs ("test/data/subs2.xml");

	auto s = subs.subtitles_during (dcp::Time (0, 0, 42, 100, 250), dcp::Time (0, 0, 42, 101, 250), false);
	BOOST_REQUIRE_EQUAL (s.size(), 2);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.front()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.front()), dcp::SubtitleString (
				   string ("theFont"),
				   true,
				   false,
				   false,
				   dcp::Colour (255, 255, 255),
				   42,
				   1.0,
				   dcp::Time (0, 0, 41, 62, 250),
				   dcp::Time (0, 0, 43, 52, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.89,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "At afternoon tea with John Peel",
				   dcp::Effect::BORDER,
				   dcp::Colour (0, 0, 0),
				   dcp::Time (0, 0, 0, 0, 250),
				   dcp::Time (0, 0, 0, 0, 250)
				   ));
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.back()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.back()), dcp::SubtitleString (
				   string ("theFont"),
				   true,
				   false,
				   false,
				   dcp::Colour (255, 255, 255),
				   42,
				   1.0,
				   dcp::Time (0, 0, 41, 62, 250),
				   dcp::Time (0, 0, 43, 52, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.95,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "I enquired if his accent was real",
				   dcp::Effect::BORDER,
				   dcp::Colour (0, 0, 0),
				   dcp::Time (0, 0, 0, 0, 250),
				   dcp::Time (0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (dcp::Time (0, 0, 50, 50, 250), dcp::Time (0, 0, 50, 51, 250), false);
	BOOST_REQUIRE_EQUAL (s.size(), 2);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.front()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.front()), dcp::SubtitleString (
				   string ("theFont"),
				   true,
				   false,
				   false,
				   dcp::Colour (255, 255, 255),
				   42,
				   1.0,
				   dcp::Time (0, 0, 50, 42, 250),
				   dcp::Time (0, 0, 52, 21, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.89,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "He said \"out of the house",
				   dcp::Effect::BORDER,
				   dcp::Colour (0, 0, 0),
				   dcp::Time (0, 0, 0, 0, 250),
				   dcp::Time (0, 0, 0, 0, 250)
				   ));
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.back()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.back()), dcp::SubtitleString (
				   string ("theFont"),
				   true,
				   false,
				   false,
				   dcp::Colour (255, 255, 255),
				   42,
				   1.0,
				   dcp::Time (0, 0, 50, 42, 250),
				   dcp::Time (0, 0, 52, 21, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.95,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "I'm incredibly scouse",
				   dcp::Effect::BORDER,
				   dcp::Colour (0, 0, 0),
				   dcp::Time (0, 0, 0, 0, 250),
				   dcp::Time (0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (dcp::Time (0, 1, 2, 300, 250), dcp::Time (0, 1, 2, 301, 250), false);
	BOOST_REQUIRE_EQUAL (s.size(), 2);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.front()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.front()), dcp::SubtitleString (
				   string("theFont"),
				   true,
				   false,
				   false,
				   dcp::Colour(255, 255, 255),
				   42,
				   1.0,
				   dcp::Time(0, 1, 2, 208, 250),
				   dcp::Time(0, 1, 4, 10, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.89,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "At home it depends how I feel.\"",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.back()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.back()), dcp::SubtitleString (
				   string ("theFont"),
				   true,
				   false,
				   false,
				   dcp::Colour(255, 255, 255),
				   42,
				   1.0,
				   dcp::Time(0, 1, 2, 208, 250),
				   dcp::Time(0, 1, 4, 10, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.95,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "I spent a long weekend in Brighton",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (dcp::Time (0, 1, 15, 50, 250), dcp::Time (0, 1, 15, 51, 250), false);
	BOOST_REQUIRE_EQUAL (s.size(), 2);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.front()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.front()), dcp::SubtitleString (
				   string ("theFont"),
				   true,
				   false,
				   false,
				   dcp::Colour(255, 255, 255),
				   42,
				   1.0,
				   dcp::Time(0, 1, 15, 42, 250),
				   dcp::Time(0, 1, 16, 42, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.89,
				   dcp::VAlign::TOP,
				   dcp::Direction::RTL,
				   "With the legendary Miss Enid Blyton",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.back()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.back()), dcp::SubtitleString (
				   string ("theFont"),
				   true,
				   false,
				   false,
				   dcp::Colour(255, 255, 255),
				   42,
				   1.0,
				   dcp::Time(0, 1, 15, 42, 250),
				   dcp::Time(0, 1, 16, 42, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.95,
				   dcp::VAlign::TOP,
				   dcp::Direction::TTB,
				   "She said \"you be Noddy",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (dcp::Time (0, 1, 27, 200, 250), dcp::Time (0, 1, 27, 201, 250), false);
	BOOST_REQUIRE_EQUAL (s.size(), 2);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.front()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.front()), dcp::SubtitleString (
				   string ("theFont"),
				   true,
				   false,
				   false,
				   dcp::Colour(255, 255, 255),
				   42,
				   1.0,
				   dcp::Time(0, 1, 27, 115, 250),
				   dcp::Time(0, 1, 28, 208, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.89,
				   dcp::VAlign::TOP,
				   dcp::Direction::BTT,
				   "That curious creature the Sphinx",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.back()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.back()), dcp::SubtitleString (
				   string ("theFont"),
				   true,
				   false,
				   false,
				   dcp::Colour(255, 255, 255),
				   42,
				   1.0,
				   dcp::Time(0, 1, 27, 115, 250),
				   dcp::Time(0, 1, 28, 208, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.95,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "Is smarter than anyone thinks",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (dcp::Time (0, 1, 42, 300, 250), dcp::Time (0, 1, 42, 301, 250), false);
	BOOST_REQUIRE_EQUAL (s.size(), 2);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.front()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.front()), dcp::SubtitleString (
				   string ("theFont"),
				   false,
				   false,
				   false,
				   dcp::Colour (255, 255, 255),
				   42,
				   1.0,
				   dcp::Time (0, 1, 42, 229, 250),
				   dcp::Time (0, 1, 45, 62, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.89,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "It sits there and smirks",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.back()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.back()), dcp::SubtitleString (
				   string ("theFont"),
				   false,
				   false,
				   false,
				   dcp::Colour (255, 255, 255),
				   42,
				   1.0,
				   dcp::Time (0, 1, 42, 229, 250),
				   dcp::Time (0, 1, 45, 62, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.95,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "And you don't think it works",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (dcp::Time (0, 1, 45, 200, 250), dcp::Time (0, 1, 45, 201, 250), false);
	BOOST_REQUIRE_EQUAL (s.size(), 2);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.front()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.front()), dcp::SubtitleString (
				   string("theFont"),
				   false,
				   false,
				   false,
				   dcp::Colour(255, 255, 255),
				   42,
				   1.0,
				   dcp::Time(0, 1, 45, 146, 250),
				   dcp::Time(0, 1, 47, 94, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.89,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "Then when you're not looking, it winks.",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.back()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.back()), dcp::SubtitleString (
				   string ("theFont"),
				   false,
				   false,
				   false,
				   dcp::Colour (255, 255, 255),
				   42,
				   1.0,
				   dcp::Time (0, 1, 45, 146, 250),
				   dcp::Time (0, 1, 47, 94, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.95,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "When it snows you will find Sister Sledge",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (dcp::Time (0, 1, 47, 249, 250), dcp::Time (0, 1, 47, 250, 250), false);
	BOOST_REQUIRE_EQUAL (s.size(), 2);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.front()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.front()), dcp::SubtitleString (
				   string ("theFont"),
				   false,
				   false,
				   false,
				   dcp::Colour(255, 255, 255),
				   42,
				   1.0,
				   dcp::Time(0, 1, 47, 146, 250),
				   dcp::Time(0, 1, 48, 167, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.89,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "Out mooning, at night, on the ledge",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.back()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.back()), dcp::SubtitleString (
				   string ("theFont"),
				   false,
				   false,
				   false,
				   dcp::Colour(255, 255, 255),
				   42,
				   1.0,
				   dcp::Time(0, 1, 47, 146, 250),
				   dcp::Time(0, 1, 48, 167, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.95,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "One storey down",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));

	s = subs.subtitles_during (dcp::Time (0, 2, 6, 210, 250), dcp::Time (0, 2, 6, 211, 250), false);
	BOOST_REQUIRE_EQUAL (s.size(), 2);
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.front()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.front()), dcp::SubtitleString (
				   string ("theFont"),
				   true,
				   false,
				   false,
				   dcp::Colour (255, 255, 255),
				   42,
				   1.0,
				   dcp::Time (0, 2, 5, 208, 250),
				   dcp::Time (0, 2, 7, 31, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.89,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "HELLO",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));
	BOOST_REQUIRE (dynamic_pointer_cast<const dcp::SubtitleString>(s.back()));
	BOOST_CHECK_EQUAL (*dynamic_pointer_cast<const dcp::SubtitleString>(s.back()), dcp::SubtitleString (
				   string ("theFont"),
				   true,
				   false,
				   false,
				   dcp::Colour(255, 255, 255),
				   42,
				   1.0,
				   dcp::Time(0, 2, 5, 208, 250),
				   dcp::Time(0, 2, 7, 31, 250),
				   0,
				   dcp::HAlign::CENTER,
				   0.95,
				   dcp::VAlign::TOP,
				   dcp::Direction::LTR,
				   "WORLD",
				   dcp::Effect::BORDER,
				   dcp::Colour(0, 0, 0),
				   dcp::Time(0, 0, 0, 0, 250),
				   dcp::Time(0, 0, 0, 0, 250)
				   ));
}

/** And one with bitmap subtitles */
BOOST_AUTO_TEST_CASE (read_interop_subtitle_test3)
{
	dcp::InteropSubtitleAsset subs ("test/data/subs3.xml");

	BOOST_REQUIRE_EQUAL (subs.subtitles().size(), 1);
	auto si = dynamic_pointer_cast<const dcp::SubtitleImage>(subs.subtitles().front());
	BOOST_REQUIRE (si);
	BOOST_CHECK (si->png_image() == dcp::ArrayData("test/data/sub.png"));
}


/** Write some subtitle content as Interop XML and check that it is right */
BOOST_AUTO_TEST_CASE (write_interop_subtitle_test)
{
	dcp::InteropSubtitleAsset c;
	c.set_reel_number ("1");
	c.set_language ("EN");
	c.set_movie_title ("Test");

	c.add (
		make_shared<dcp::SubtitleString>(
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
			dcp::Direction::LTR,
			"What's going on",
			dcp::Effect::BORDER,
			dcp::Colour (1, 2, 3),
			dcp::Time (1, 2, 3, 4, 24),
			dcp::Time (5, 6, 7, 8, 24)
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
		make_shared<dcp::SubtitleString>(
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
		);

	c.add (
		make_shared<dcp::SubtitleString>(
			boost::optional<string>(),
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

	auto c = make_shared<dcp::InteropSubtitleAsset>();
	c->set_reel_number ("1");
	c->set_language ("EN");
	c->set_movie_title ("Test");

	c->add (
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

	c->_id = "a6c58cff-3e1e-4b38-acec-a42224475ef6";
	boost::filesystem::remove_all ("build/test/write_interop_subtitle_test3");
	boost::filesystem::create_directories ("build/test/write_interop_subtitle_test3");
	c->write ("build/test/write_interop_subtitle_test3/subs.xml");

	auto reel = make_shared<dcp::Reel>();
	reel->add(make_shared<dcp::ReelInteropSubtitleAsset>(c, dcp::Fraction(24, 1), 6046, 0));

	string const issue_date = "2018-09-02T04:45:18+00:00";
	string const issuer = "libdcp";
	string const creator = "libdcp";
	string const annotation_text = "Created by libdcp";

	auto cpl = make_shared<dcp::CPL>("My film", dcp::ContentKind::FEATURE, dcp::Standard::INTEROP);
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
	dcp.write_xml (issuer, creator, issue_date, annotation_text);

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
		dcp::file_to_string(find_file("test/ref/write_interop_subtitle_test3", "pkl")),
		dcp::file_to_string(find_file("build/test/write_interop_subtitle_test3", "pkl")),
		vector<string>()
		);
}

