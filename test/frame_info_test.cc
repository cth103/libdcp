/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

#include <fstream>
#include <boost/test/unit_test.hpp>
#include "picture_asset_writer.h"

using namespace std;

/* Test writing and reading of frame_info_test with fstream and stdio */
BOOST_AUTO_TEST_CASE (frame_info_test)
{
	libdcp::FrameInfo a (8589934592LL, 17179869184LL, "thisisahash");

	ofstream o1 ("build/test/frame_info1");
	a.write (o1);
	o1.close ();

	FILE* o2 = fopen ("build/test/frame_info2", "w");
	BOOST_CHECK (o2);
	a.write (o2);
	fclose (o2);

	ifstream c1 ("build/test/frame_info1");
	string s1;
	getline (c1, s1);

	ifstream c2 ("build/test/frame_info2");
	string s2;
	getline (c2, s2);

	BOOST_CHECK_EQUAL (s1, s2);

	ifstream l1 ("build/test/frame_info1");
	libdcp::FrameInfo b1 (l1);

	FILE* l2 = fopen ("build/test/frame_info2", "r");
	BOOST_CHECK (l2);
	libdcp::FrameInfo b2 (l2);

	BOOST_CHECK_EQUAL (b1.offset, b2.offset);
	BOOST_CHECK_EQUAL (b1.size, b2.size);
	BOOST_CHECK_EQUAL (b1.hash, b2.hash);
}
