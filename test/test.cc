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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libdcp_test
#include <boost/test/unit_test.hpp>
#include "util.h"

using std::string;

string private_test;

struct TestConfig
{
	TestConfig()
	{
		libdcp::init ();
		if (boost::unit_test::framework::master_test_suite().argc >= 2) {
			private_test = boost::unit_test::framework::master_test_suite().argv[1];
		}
	}
};

BOOST_GLOBAL_FIXTURE (TestConfig);

boost::filesystem::path
j2c (int)
{
	return "test/data/32x32_red_square.j2c";
}

boost::filesystem::path
wav (libdcp::Channel)
{
	return "test/data/1s_24-bit_48k_silence.wav";
}


