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
#include "util.h"

using std::ifstream;
using std::string;

BOOST_AUTO_TEST_CASE (base64_decode_test)
{
	int const N = 256;
	
	ifstream f ("test/data/base64_test");
	BOOST_CHECK (f.good ());
	string s;
	while (f.good ()) {
		string l;
		getline (f, l);
		s += l;
	}

	ifstream g ("test/ref/base64_test_decoded", std::ios::binary);
	BOOST_CHECK (g.good ());
	unsigned char ref_decoded[N];
	for (int i = 0; i < N; ++i) {
		char c;
		g.get (c);
		ref_decoded[i] = static_cast<unsigned char> (c);
	}

	unsigned char decoded[N];
	int const r = libdcp::base64_decode (s, decoded, N);
	BOOST_CHECK_EQUAL (r, N);

	for (int i = 0; i < N; ++i) {
		BOOST_CHECK_EQUAL (decoded[i], ref_decoded[i]);
	}
}

BOOST_AUTO_TEST_CASE (content_kind_test)
{
	BOOST_CHECK_EQUAL (libdcp::content_kind_from_string ("feature"), libdcp::FEATURE);
	BOOST_CHECK_EQUAL (libdcp::content_kind_from_string ("Feature"), libdcp::FEATURE);
	BOOST_CHECK_EQUAL (libdcp::content_kind_from_string ("FeaturE"), libdcp::FEATURE);
	BOOST_CHECK_EQUAL (libdcp::content_kind_from_string ("Short"), libdcp::SHORT);
	BOOST_CHECK_EQUAL (libdcp::content_kind_from_string ("trailer"), libdcp::TRAILER);
	BOOST_CHECK_EQUAL (libdcp::content_kind_from_string ("test"), libdcp::TEST);
	BOOST_CHECK_EQUAL (libdcp::content_kind_from_string ("transitional"), libdcp::TRANSITIONAL);
	BOOST_CHECK_EQUAL (libdcp::content_kind_from_string ("rating"), libdcp::RATING);
	BOOST_CHECK_EQUAL (libdcp::content_kind_from_string ("teaser"), libdcp::TEASER);
	BOOST_CHECK_EQUAL (libdcp::content_kind_from_string ("policy"), libdcp::POLICY);
	BOOST_CHECK_EQUAL (libdcp::content_kind_from_string ("psa"), libdcp::PUBLIC_SERVICE_ANNOUNCEMENT);
	BOOST_CHECK_EQUAL (libdcp::content_kind_from_string ("advertisement"), libdcp::ADVERTISEMENT);
}
