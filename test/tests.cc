/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#include <boost/filesystem.hpp>
#include "KM_prng.h"
#include "dcp.h"
#include "util.h"
#include "metadata.h"
#include "types.h"
#include "exceptions.h"
#include "subtitle_asset.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libdcp_test
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace boost;

string
j2c (int)
{
	return "test/data/32x32_red_square.j2c";
}

string
wav (libdcp::Channel)
{
	return "test/data/1s_24-bit_48k_silence.wav";
}
		

BOOST_AUTO_TEST_CASE (dcp_test)
{
	Kumu::libdcp_test = true;
	
	libdcp::Metadata* t = libdcp::Metadata::instance ();
	t->issuer = "OpenDCP 0.0.25";
	t->creator = "OpenDCP 0.0.25";
	t->company_name = "OpenDCP";
	t->product_name = "OpenDCP";
	t->product_version = "0.0.25";
	t->issue_date = "2012-07-17T04:45:18+00:00";
	filesystem::remove_all ("build/test/foo");
	filesystem::create_directories ("build/test/foo");
	libdcp::DCP d ("build/test/foo", "A Test DCP", libdcp::FEATURE, 24, 24);

	d.add_picture_asset (sigc::ptr_fun (&j2c), 32, 32);
	d.add_sound_asset (sigc::ptr_fun (&wav), 2);

	d.write_xml ();
}

BOOST_AUTO_TEST_CASE (error_test)
{
	libdcp::DCP d ("build/test/bar", "A Test DCP", libdcp::TEST, 24, 24);
	vector<string> p;
	p.push_back ("frobozz");
	BOOST_CHECK_THROW (d.add_picture_asset (p, 32, 32), libdcp::FileError);
	BOOST_CHECK_THROW (d.add_sound_asset (p), libdcp::FileError);
}

BOOST_AUTO_TEST_CASE (read_dcp)
{
	libdcp::DCP d ("test/ref/DCP");

	BOOST_CHECK_EQUAL (d.name(), "A Test DCP");
	BOOST_CHECK_EQUAL (d.content_kind(), libdcp::FEATURE);
	BOOST_CHECK_EQUAL (d.frames_per_second(), 24);
	BOOST_CHECK_EQUAL (d.length(), 24);
}
	
BOOST_AUTO_TEST_CASE (subtitles)
{
	libdcp::SubtitleAsset subs ("test/ref", "subs.xml");

	BOOST_CHECK_EQUAL (subs.language(), "French");
	BOOST_CHECK_EQUAL (subs.fonts().size(), 1);
	BOOST_CHECK_EQUAL (subs.fonts().front()->subtitles().size(), 4);

	list<shared_ptr<libdcp::Subtitle> >::const_iterator i = subs.fonts().front()->subtitles().begin ();

	BOOST_CHECK_EQUAL ((*i)->in(), libdcp::Time (0, 0, 5, 198));
	BOOST_CHECK_EQUAL ((*i)->out(), libdcp::Time (0, 0, 7, 115));
	BOOST_CHECK_EQUAL ((*i)->texts().size(), 1);
	BOOST_CHECK_EQUAL ((*i)->texts().front()->v_position(), 15);
	BOOST_CHECK_EQUAL ((*i)->texts().front()->text(), "My jacket was Idi Amin's");
	++i;

	BOOST_CHECK_EQUAL ((*i)->in(), libdcp::Time (0, 0, 7, 177));
	BOOST_CHECK_EQUAL ((*i)->out(), libdcp::Time (0, 0, 11, 31));
	BOOST_CHECK_EQUAL ((*i)->texts().size(), 2);
	BOOST_CHECK_EQUAL ((*i)->texts().front()->v_position(), 21);
	BOOST_CHECK_EQUAL ((*i)->texts().front()->text(), "My corset was H.M. The Queen's");
	BOOST_CHECK_EQUAL ((*i)->texts().back()->v_position(), 15);
	BOOST_CHECK_EQUAL ((*i)->texts().back()->text(), "My large wonderbra");
	++i;

	BOOST_CHECK_EQUAL ((*i)->in(), libdcp::Time (0, 0, 11, 94));
	BOOST_CHECK_EQUAL ((*i)->out(), libdcp::Time (0, 0, 13, 63));
	BOOST_CHECK_EQUAL ((*i)->texts().size(), 1);
	BOOST_CHECK_EQUAL ((*i)->texts().front()->v_position(), 15);
	BOOST_CHECK_EQUAL ((*i)->texts().front()->text(), "Once belonged to the Shah");
	++i;

	BOOST_CHECK_EQUAL ((*i)->in(), libdcp::Time (0, 0, 13, 104));
	BOOST_CHECK_EQUAL ((*i)->out(), libdcp::Time (0, 0, 15, 177));
	BOOST_CHECK_EQUAL ((*i)->texts().size(), 1);
	BOOST_CHECK_EQUAL ((*i)->texts().front()->v_position(), 15);
	BOOST_CHECK_EQUAL ((*i)->texts().front()->text(), "And these are Roy Hattersley's jeans");

	BOOST_CHECK_EQUAL (subs.subtitles_at (libdcp::Time (0, 0, 14, 042)).size(), 1);
	BOOST_CHECK_EQUAL (subs.subtitles_at (libdcp::Time (0, 0, 14, 042)).front()->text(), "And these are Roy Hattersley's jeans");
}

BOOST_AUTO_TEST_CASE (dcp_time)
{
	libdcp::Time t (977143, 24);

	BOOST_CHECK_EQUAL (t.ms, 292);
	BOOST_CHECK_EQUAL (t.s, 34);
	BOOST_CHECK_EQUAL (t.m, 18);
	BOOST_CHECK_EQUAL (t.h, 11);
}
