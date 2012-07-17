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
#include "tags.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libdcp_test
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace boost;

BOOST_AUTO_TEST_CASE (dcp_test)
{
	Kumu::libdcp_test = true;
	
	libdcp::Tags* t = libdcp::Tags::instance ();
	t->issuer = "OpenDCP 0.0.25";
	t->creator = "OpenDCP 0.0.25";
	t->company_name = "OpenDCP";
	t->product_name = "OpenDCP";
	t->product_version = "0.0.25";
	
	filesystem::remove_all ("build/test/foo");
	filesystem::create_directories ("build/test/foo");
	libdcp::DCP d ("build/test/foo", "A Test DCP", libdcp::DCP::FEATURE, 24, 24);

	list<string> j2cs;
	for (int i = 0; i < 24; ++i) {
		j2cs.push_back ("test/data/32x32_red_square.j2c");
	}
	d.add_picture_asset (j2cs, 32, 32);
	
	list<string> wavs;
	for (int i = 0; i < 2; ++i) {
		wavs.push_back ("test/data/1s_24-bit_48k_silence.wav");
	}
	d.add_sound_asset (wavs);

	d.write_xml ();
}
