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

#include "smpte_subtitle_asset.h"
#include "test.h"
#include "local_time.h"
#include "smpte_load_font_node.h"
#include <boost/test/unit_test.hpp>

using std::list;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;

/** Check reading of a SMPTE subtitle file */
BOOST_AUTO_TEST_CASE (read_smpte_subtitle_test)
{
	dcp::SMPTESubtitleAsset sc (private_test / "8dfafe11-2bd1-4206-818b-afc109cfe7f6_reel1.xml", false);

	BOOST_CHECK_EQUAL (sc.id(), "8dfafe11-2bd1-4206-818b-afc109cfe7f6");
	BOOST_CHECK_EQUAL (sc.content_title_text(), "Violet");
	BOOST_REQUIRE (sc.annotation_text());
	BOOST_CHECK_EQUAL (sc.annotation_text().get(), "Violet");
	BOOST_CHECK_EQUAL (sc.issue_date(), dcp::LocalTime ("2014-12-23T22:30:07.000-00:00"));
	BOOST_REQUIRE (sc.reel_number());
	BOOST_CHECK_EQUAL (sc.reel_number().get(), 1);
	BOOST_REQUIRE (sc.language ());
	BOOST_CHECK_EQUAL (sc.language().get (), "Dutch");
	BOOST_CHECK_EQUAL (sc.edit_rate(), dcp::Fraction (24, 1));
	BOOST_CHECK_EQUAL (sc.time_code_rate(), 24);
	BOOST_CHECK_EQUAL (sc.start_time(), dcp::Time (0, 0, 0, 23, 24));
	list<shared_ptr<dcp::LoadFontNode> > lfn = sc.load_font_nodes ();
	BOOST_REQUIRE_EQUAL (lfn.size(), 1);
	shared_ptr<dcp::SMPTELoadFontNode> smpte_lfn = dynamic_pointer_cast<dcp::SMPTELoadFontNode> (lfn.front ());
	BOOST_REQUIRE (smpte_lfn);
	BOOST_CHECK_EQUAL (smpte_lfn->id, "theFontId");
	BOOST_CHECK_EQUAL (smpte_lfn->urn, "3dec6dc0-39d0-498d-97d0-928d2eb78391");
	BOOST_REQUIRE_EQUAL (sc.subtitles().size(), 159);
	BOOST_CHECK_EQUAL (sc.subtitles().front().text(), "Jonas ?");
	BOOST_CHECK_EQUAL (sc.subtitles().front().in(), dcp::Time (0, 7, 6, 20, 24));
	BOOST_CHECK_EQUAL (sc.subtitles().front().out(), dcp::Time (0, 7, 7, 20, 24));
	BOOST_CHECK_EQUAL (sc.subtitles().back().text(), "Come on.");
	BOOST_CHECK_EQUAL (sc.subtitles().back().in(), dcp::Time (1, 13, 37, 11, 24));
	BOOST_CHECK_EQUAL (sc.subtitles().back().out(), dcp::Time (1, 13, 38, 11, 24));
}
