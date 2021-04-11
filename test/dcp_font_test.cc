/*
    Copyright (C) 2015-2019 Carl Hetherington <cth@carlh.net>

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
#include "dcp.h"
#include "cpl.h"
#include "test.h"
#include "reel.h"
#include "util.h"
#include "reel_interop_subtitle_asset.h"
#include "reel_smpte_subtitle_asset.h"
#include <boost/test/unit_test.hpp>
#include <cstdio>


using std::list;
using std::string;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using std::make_shared;
using boost::shared_array;

/** Create a DCP with interop subtitles and check that the font is written and read back correctly */
BOOST_AUTO_TEST_CASE (interop_dcp_font_test)
{
	boost::filesystem::path directory = "build/test/interop_dcp_font_test";
	dcp::DCP dcp (directory);

	auto subs = make_shared<dcp::InteropSubtitleAsset>();
	subs->add_font ("theFontId", dcp::ArrayData("test/data/dummy.ttf"));
	subs->write (directory / "frobozz.xml");
	check_file ("test/data/dummy.ttf", "build/test/interop_dcp_font_test/font_0.ttf");

	auto reel = make_shared<dcp::Reel>();
	reel->add (make_shared<dcp::ReelInteropSubtitleAsset>(subs, dcp::Fraction (24, 1), 24, 0));

	auto cpl = make_shared<dcp::CPL>("", dcp::ContentKind::TRAILER, dcp::Standard::INTEROP);
	cpl->add (reel);

	dcp.add (cpl);
	dcp.write_xml ();

	dcp::DCP dcp2 (directory);
	dcp2.read ();
	auto subs2 = dynamic_pointer_cast<dcp::SubtitleAsset> (
		dcp2.cpls()[0]->reels()[0]->main_subtitle()->asset_ref().asset()
		);
	BOOST_REQUIRE (subs2);
	BOOST_REQUIRE_EQUAL (subs2->_fonts.size(), 1);

	auto const size = boost::filesystem::file_size ("test/data/dummy.ttf");
	auto f = dcp::fopen_boost ("test/data/dummy.ttf", "rb");
	BOOST_REQUIRE (f);
	shared_array<uint8_t> ref (new uint8_t[size]);
	fread (ref.get(), 1, size, f);
	fclose (f);

	BOOST_CHECK_EQUAL (memcmp (subs2->_fonts.front().data.data(), ref.get(), size), 0);
}

/** Create a DCP with SMPTE subtitles and check that the font is written and read back correctly */
BOOST_AUTO_TEST_CASE (smpte_dcp_font_test)
{
	boost::filesystem::path directory = "build/test/smpte_dcp_font_test";
	dcp::DCP dcp (directory);

	auto subs = make_shared<dcp::SMPTESubtitleAsset>();
	subs->add_font ("theFontId", dcp::ArrayData("test/data/dummy.ttf"));
	subs->write (directory / "frobozz.mxf");

	auto reel = make_shared<dcp::Reel>();
	reel->add (make_shared<dcp::ReelSMPTESubtitleAsset>(subs, dcp::Fraction (24, 1), 24, 0));

	auto cpl = make_shared<dcp::CPL>("", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->add (reel);

	dcp.add (cpl);
	dcp.write_xml ();

	dcp::DCP dcp2 (directory);
	dcp2.read ();
	auto subs2 = dynamic_pointer_cast<dcp::SubtitleAsset> (
		dcp2.cpls().front()->reels().front()->main_subtitle()->asset_ref().asset()
		);
	BOOST_REQUIRE (subs2);
	BOOST_REQUIRE_EQUAL (subs2->_fonts.size(), 1);

	auto const size = boost::filesystem::file_size ("test/data/dummy.ttf");
	auto f = dcp::fopen_boost ("test/data/dummy.ttf", "rb");
	BOOST_REQUIRE (f);
	shared_array<uint8_t> ref (new uint8_t[size]);
	fread (ref.get(), 1, size, f);
	fclose (f);

	BOOST_REQUIRE (subs2->_fonts.front().data.data());
	BOOST_CHECK_EQUAL (memcmp (subs2->_fonts.front().data.data(), ref.get(), size), 0);
}
