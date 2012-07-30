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
	
	
