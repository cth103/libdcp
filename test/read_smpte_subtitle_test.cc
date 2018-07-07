/*
    Copyright (C) 2015 Carl Hetherington <cth@carlh.net>

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

#include "smpte_subtitle_asset.h"
#include "test.h"
#include "local_time.h"
#include "smpte_load_font_node.h"
#include <boost/test/unit_test.hpp>
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

using std::list;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;

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
	list<shared_ptr<dcp::LoadFontNode> > lfn = sc.load_font_nodes ();
	BOOST_REQUIRE_EQUAL (lfn.size(), 1);
	shared_ptr<dcp::SMPTELoadFontNode> smpte_lfn = dynamic_pointer_cast<dcp::SMPTELoadFontNode> (lfn.front ());
	BOOST_REQUIRE (smpte_lfn);
	BOOST_CHECK_EQUAL (smpte_lfn->id, "theFontId");
	BOOST_CHECK_EQUAL (smpte_lfn->urn, "9118bbce-4105-4a05-b37c-a5a6f75e1fea");
	BOOST_REQUIRE_EQUAL (sc.subtitles().size(), 63);
	BOOST_REQUIRE (dynamic_pointer_cast<dcp::SubtitleString>(sc.subtitles().front()));
	BOOST_CHECK_EQUAL (dynamic_pointer_cast<dcp::SubtitleString>(sc.subtitles().front())->text(), "Noch mal.");
	BOOST_CHECK_EQUAL (sc.subtitles().front()->in(), dcp::Time (0, 0, 25, 12, 25));
	BOOST_CHECK_EQUAL (sc.subtitles().front()->out(), dcp::Time (0, 0, 26, 4, 25));
	BOOST_REQUIRE (dynamic_pointer_cast<dcp::SubtitleString>(sc.subtitles().back()));
	BOOST_CHECK_EQUAL (dynamic_pointer_cast<dcp::SubtitleString>(sc.subtitles().back())->text(), "Prochainement");
	BOOST_CHECK_EQUAL (sc.subtitles().back()->in(), dcp::Time (0, 1, 57, 17, 25));
	BOOST_CHECK_EQUAL (sc.subtitles().back()->out(), dcp::Time (0, 1, 58, 12, 25));
}

/** And another one featuring <Font> within <Text> */
BOOST_AUTO_TEST_CASE (read_smpte_subtitle_test2)
{
	dcp::SMPTESubtitleAsset sc (private_test / "olsson.xml");

	BOOST_REQUIRE_EQUAL (sc.subtitles().size(), 6);
	list<shared_ptr<dcp::Subtitle> >::const_iterator i = sc.subtitles().begin();
	shared_ptr<dcp::SubtitleString> is = dynamic_pointer_cast<dcp::SubtitleString>(*i);
	BOOST_REQUIRE (is);
	BOOST_CHECK_EQUAL (is->text(), "Testing is ");
	BOOST_CHECK (!is->italic());
	++i;
	is = dynamic_pointer_cast<dcp::SubtitleString>(*i);
	BOOST_REQUIRE (is);
	BOOST_CHECK_EQUAL (is->text(), "really");
	BOOST_CHECK (is->italic());
	++i;
	is = dynamic_pointer_cast<dcp::SubtitleString>(*i);
	BOOST_REQUIRE (is);
	BOOST_CHECK_EQUAL (is->text(), " fun!");
	BOOST_CHECK (!is->italic());
	++i;
	is = dynamic_pointer_cast<dcp::SubtitleString>(*i);
	BOOST_REQUIRE (is);
	BOOST_CHECK_EQUAL (is->text(), "This is the ");
	BOOST_CHECK (!is->italic());
	++i;
	is = dynamic_pointer_cast<dcp::SubtitleString>(*i);
	BOOST_REQUIRE (is);
	BOOST_CHECK_EQUAL (is->text(), "second");
	BOOST_CHECK (is->italic());
	++i;
	is = dynamic_pointer_cast<dcp::SubtitleString>(*i);
	BOOST_REQUIRE (is);
	BOOST_CHECK_EQUAL (is->text(), " line!");
	BOOST_CHECK (!is->italic());
}
