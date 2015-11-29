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

#include "interop_subtitle_asset.h"
#include "smpte_subtitle_asset.h"
#include "dcp.h"
#include "cpl.h"
#include "test.h"
#include "reel.h"
#include "util.h"
#include "reel_subtitle_asset.h"
#include <boost/test/unit_test.hpp>
#include <cstdio>

using std::list;
using std::string;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using boost::shared_array;

/** Create a DCP with interop subtitles and check that the font is written and read back correctly */
BOOST_AUTO_TEST_CASE (interop_dcp_font_test)
{
	boost::filesystem::path directory = "build/test/interop_dcp_font_test";
	dcp::DCP dcp (directory);

	shared_ptr<dcp::InteropSubtitleAsset> subs (new dcp::InteropSubtitleAsset ());
	subs->add_font ("theFontId", "test/data/dummy.ttf");
	subs->write (directory / "frobozz.xml");
	check_file ("test/data/dummy.ttf", "build/test/interop_dcp_font_test/dummy.ttf");

	shared_ptr<dcp::Reel> reel (new dcp::Reel ());
	reel->add (shared_ptr<dcp::ReelAsset> (new dcp::ReelSubtitleAsset (subs, dcp::Fraction (24, 1), 24, 0)));

	shared_ptr<dcp::CPL> cpl (new dcp::CPL ("", dcp::TRAILER));
	cpl->add (reel);

	dcp.add (cpl);
	dcp.write_xml (dcp::INTEROP);

	dcp::DCP dcp2 (directory);
	dcp2.read ();
	shared_ptr<dcp::SubtitleAsset> subs2 = dynamic_pointer_cast<dcp::SubtitleAsset> (
		dcp2.cpls().front()->reels().front()->main_subtitle()->asset_ref().object()
		);
	BOOST_REQUIRE (subs2);
	BOOST_REQUIRE_EQUAL (subs2->_fonts.size(), 1);

	boost::uintmax_t const size = boost::filesystem::file_size ("test/data/dummy.ttf");
	FILE* f = dcp::fopen_boost ("test/data/dummy.ttf", "rb");
	BOOST_REQUIRE (f);
	shared_array<uint8_t> ref (new uint8_t[size]);
	fread (ref.get(), 1, size, f);
	fclose (f);

	BOOST_CHECK_EQUAL (memcmp (subs2->_fonts.front().data.data().get(), ref.get(), size), 0);
}

/** Create a DCP with SMPTE subtitles and check that the font is written and read back correctly */
BOOST_AUTO_TEST_CASE (smpte_dcp_font_test)
{
	boost::filesystem::path directory = "build/test/smpte_dcp_font_test";
	dcp::DCP dcp (directory);

	shared_ptr<dcp::SMPTESubtitleAsset> subs (new dcp::SMPTESubtitleAsset ());
	subs->add_font ("theFontId", "test/data/dummy.ttf");
	subs->write (directory / "frobozz.mxf");

	shared_ptr<dcp::Reel> reel (new dcp::Reel ());
	reel->add (shared_ptr<dcp::ReelAsset> (new dcp::ReelSubtitleAsset (subs, dcp::Fraction (24, 1), 24, 0)));

	shared_ptr<dcp::CPL> cpl (new dcp::CPL ("", dcp::TRAILER));
	cpl->add (reel);

	dcp.add (cpl);
	dcp.write_xml (dcp::SMPTE);

	dcp::DCP dcp2 (directory);
	dcp2.read ();
	shared_ptr<dcp::SubtitleAsset> subs2 = dynamic_pointer_cast<dcp::SubtitleAsset> (
		dcp2.cpls().front()->reels().front()->main_subtitle()->asset_ref().object()
		);
	BOOST_REQUIRE (subs2);
	BOOST_REQUIRE_EQUAL (subs2->_fonts.size(), 1);

	boost::uintmax_t const size = boost::filesystem::file_size ("test/data/dummy.ttf");
	FILE* f = dcp::fopen_boost ("test/data/dummy.ttf", "rb");
	BOOST_REQUIRE (f);
	shared_array<uint8_t> ref (new uint8_t[size]);
	fread (ref.get(), 1, size, f);
	fclose (f);

	BOOST_REQUIRE (subs2->_fonts.front().data.data());
	BOOST_CHECK_EQUAL (memcmp (subs2->_fonts.front().data.data().get(), ref.get(), size), 0);
}
