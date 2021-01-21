/*
    Copyright (C) 2013-2019 Carl Hetherington <cth@carlh.net>

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

#include "util.h"
#include "local_time.h"
#include "stream_operators.h"
#include <boost/test/unit_test.hpp>
#include <fstream>

using std::ifstream;
using std::string;
using std::vector;

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
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("feature"), dcp::ContentKind::FEATURE);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("Feature"), dcp::ContentKind::FEATURE);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("FeaturE"), dcp::ContentKind::FEATURE);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("Short"), dcp::ContentKind::SHORT);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("trailer"), dcp::ContentKind::TRAILER);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("test"), dcp::ContentKind::TEST);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("transitional"), dcp::ContentKind::TRANSITIONAL);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("rating"), dcp::ContentKind::RATING);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("teaser"), dcp::ContentKind::TEASER);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("policy"), dcp::ContentKind::POLICY);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("psa"), dcp::ContentKind::PUBLIC_SERVICE_ANNOUNCEMENT);
	BOOST_CHECK_EQUAL (dcp::content_kind_from_string ("advertisement"), dcp::ContentKind::ADVERTISEMENT);
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

BOOST_AUTO_TEST_CASE (day_less_than_or_equal_test)
{
	{
		/* equal */
		dcp::LocalTime a ("1978-04-05T00:00:00");
		dcp::LocalTime b ("1978-04-05T00:00:00");
		BOOST_CHECK (day_less_than_or_equal(a, b));
	}

	{
		/* every part of a less than b */
		dcp::LocalTime a ("1981-02-04T00:00:00");
		dcp::LocalTime b ("1985-05-23T00:00:00");
		BOOST_CHECK (day_less_than_or_equal(a, b));
	}

	{
		/* years equal, other parts less */
		dcp::LocalTime a ("1981-03-02T00:00:00");
		dcp::LocalTime b ("1981-05-10T00:00:00");
		BOOST_CHECK (day_less_than_or_equal(a, b));
	}

	{
		/* year and month equal, day less */
		dcp::LocalTime a ("1981-03-09T00:00:00");
		dcp::LocalTime b ("1981-03-12T00:00:00");
		BOOST_CHECK (day_less_than_or_equal(a, b));
	}

	{
		/* a one day later than b */
		dcp::LocalTime a ("1981-03-05T00:00:00");
		dcp::LocalTime b ("1981-03-04T00:00:00");
		BOOST_CHECK (!day_less_than_or_equal(a, b));
	}

	{
		/* year and month same, day much later */
		dcp::LocalTime a ("1981-03-22T00:00:00");
		dcp::LocalTime b ("1981-03-04T00:00:00");
		BOOST_CHECK (!day_less_than_or_equal(a, b));
	}

	{
		/* year same, month and day later */
		dcp::LocalTime a ("1981-06-22T00:00:00");
		dcp::LocalTime b ("1981-02-04T00:00:00");
		BOOST_CHECK (!day_less_than_or_equal(a, b));
	}

	{
		/* all later */
		dcp::LocalTime a ("1999-06-22T00:00:00");
		dcp::LocalTime b ("1981-02-04T00:00:00");
		BOOST_CHECK (!day_less_than_or_equal(a, b));
	}
}

BOOST_AUTO_TEST_CASE (day_greater_than_or_equal_test)
{
	{
		/* equal */
		dcp::LocalTime a ("1978-04-05T00:00:00");
		dcp::LocalTime b ("1978-04-05T00:00:00");
		BOOST_CHECK (day_greater_than_or_equal(a, b));
	}

	{
		/* every part of a less than b */
		dcp::LocalTime a ("1981-03-04T00:00:00");
		dcp::LocalTime b ("1985-05-23T00:00:00");
		BOOST_CHECK (!day_greater_than_or_equal(a, b));
	}

	{
		/* years equal, other parts less */
		dcp::LocalTime a ("1981-02-05T00:00:00");
		dcp::LocalTime b ("1981-05-10T00:00:00");
		BOOST_CHECK (!day_greater_than_or_equal(a, b));
	}

	{
		/* year and month equal, day less */
		dcp::LocalTime a ("1981-03-04T00:00:00");
		dcp::LocalTime b ("1981-03-12T00:00:00");
		BOOST_CHECK (!day_greater_than_or_equal(a, b));
	}

	{
		/* year and month equal, day less */
		dcp::LocalTime a ("1981-03-01T00:00:00");
		dcp::LocalTime b ("1981-03-04T00:00:00");
		BOOST_CHECK (!day_greater_than_or_equal(a, b));
	}

	{
		/* a one day later than b */
		dcp::LocalTime a ("1981-03-05T00:00:00");
		dcp::LocalTime b ("1981-03-04T00:00:00");
		BOOST_CHECK (day_greater_than_or_equal(a, b));
	}

	{
		/* year and month same, day much later */
		dcp::LocalTime a ("1981-03-22T00:00:00");
		dcp::LocalTime b ("1981-03-04T00:00:00");
		BOOST_CHECK (day_greater_than_or_equal(a, b));
	}

	{
		/* year same, month and day later */
		dcp::LocalTime a ("1981-05-22T00:00:00");
		dcp::LocalTime b ("1981-02-04T00:00:00");
		BOOST_CHECK (day_greater_than_or_equal(a, b));
	}

	{
		/* all later */
		dcp::LocalTime a ("1999-06-22T00:00:00");
		dcp::LocalTime b ("1981-02-04T00:00:00");
		BOOST_CHECK (day_greater_than_or_equal(a, b));
	}
}

BOOST_AUTO_TEST_CASE (unique_string_test)
{
	vector<string> existing;
	for (int i = 0; i < 16; i++) {
		string s;
		BOOST_CHECK_NO_THROW (s = dcp::unique_string(existing, "foo"));
		BOOST_CHECK (find(existing.begin(), existing.end(), s) == existing.end());
		existing.push_back (s);
	}
}
