/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

#include <fstream>
#include <boost/test/unit_test.hpp>
#include "util.h"

using std::ifstream;
using std::string;
using std::list;

/** Test dcp::base64_decode */
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
	int const r = dcp::base64_decode (s, decoded, N);
	BOOST_CHECK_EQUAL (r, N);

	for (int i = 0; i < N; ++i) {
		BOOST_CHECK_EQUAL (decoded[i], ref_decoded[i]);
	}
}

/** Test dcp::content_kind_from_string */
BOOST_AUTO_TEST_CASE (content_kind_test)
{
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("feature"), dcp::FEATURE);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("Feature"), dcp::FEATURE);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("FeaturE"), dcp::FEATURE);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("Short"), dcp::SHORT);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("trailer"), dcp::TRAILER);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("test"), dcp::TEST);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("transitional"), dcp::TRANSITIONAL);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("rating"), dcp::RATING);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("teaser"), dcp::TEASER);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("policy"), dcp::POLICY);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("psa"), dcp::PUBLIC_SERVICE_ANNOUNCEMENT);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("advertisement"), dcp::ADVERTISEMENT);
}

/** Test dcp::relative_to_root */
BOOST_AUTO_TEST_CASE (relative_to_root_test)
{
	{
		boost::filesystem::path root = "a";
		root /= "b";

		boost::filesystem::path file = "a";
		file /= "b";
		file /= "c";

		boost::optional<boost::filesystem::path> rel = dcp::relative_to_root (root, file);
		BOOST_CHECK (rel);
		BOOST_CHECK_EQUAL (rel.get(), boost::filesystem::path ("c"));
	}

	{
		boost::filesystem::path root = "a";
		root /= "b";
		root /= "c";

		boost::filesystem::path file = "a";
		file /= "b";

		boost::optional<boost::filesystem::path> rel = dcp::relative_to_root (root, file);
		BOOST_CHECK (!rel);
	}

	{
		boost::filesystem::path root = "a";

		boost::filesystem::path file = "a";
		file /= "b";
		file /= "c";

		boost::optional<boost::filesystem::path> rel = dcp::relative_to_root (root, file);
		BOOST_CHECK (rel);

		boost::filesystem::path check = "b";
		check /= "c";
		BOOST_CHECK_EQUAL (rel.get(), check);
	}
}

/** Test private_key_fingerprint() */
BOOST_AUTO_TEST_CASE (private_key_fingerprint_test)
{
	BOOST_CHECK_EQUAL (dcp::private_key_fingerprint (dcp::file_to_string ("test/data/private.key")), "Jdz1bFpCcKI7R16Ccx9JHYytag0=");
}

BOOST_AUTO_TEST_CASE (unique_string_test)
{
	list<string> existing;
	for (int i = 0; i < 16; i++) {
		string s;
		BOOST_CHECK_NO_THROW (s = dcp::unique_string(existing, "foo"));
		BOOST_CHECK (find(existing.begin(), existing.end(), s) == existing.end());
		existing.push_back (s);
	}
}
