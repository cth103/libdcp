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

#include <boost/test/unit_test.hpp>
#include "dcp_time.h"
#include "exceptions.h"

using boost::optional;

/** Check that dcp::Time works */
BOOST_AUTO_TEST_CASE (dcp_time)
{
	/* tcr of 250 makes the editable event length the same as an Interop `tick' */
	dcp::Time t (977143, 24, 250);

	BOOST_CHECK_EQUAL (t.e, 73);
	BOOST_CHECK_EQUAL (t.s, 34);
	BOOST_CHECK_EQUAL (t.m, 18);
	BOOST_CHECK_EQUAL (t.h, 11);
	BOOST_CHECK_EQUAL (t.as_string(dcp::Standard::INTEROP), "11:18:34:073");

	/* Use a tcr of 24 so that the editable event is a frame */
	dcp::Time a (3, 2, 3, 4, 24);
	dcp::Time b (2, 3, 4, 5, 24);

	dcp::Time r = a - b;
	BOOST_CHECK_EQUAL (r.h, 0);
	BOOST_CHECK_EQUAL (r.m, 58);
	BOOST_CHECK_EQUAL (r.s, 58);
	BOOST_CHECK_EQUAL (r.e, 23);
	BOOST_CHECK_EQUAL (r.as_string(dcp::Standard::INTEROP), "00:58:58:023");

	/* Different tcr (25) */
	a = dcp::Time (1, 58, 56, 2, 25);
	b = dcp::Time (1, 7, 12, 1, 25);
	r = a + b;
	BOOST_CHECK_EQUAL (r.h, 3);
	BOOST_CHECK_EQUAL (r.m, 6);
	BOOST_CHECK_EQUAL (r.s, 8);
	BOOST_CHECK_EQUAL (r.e, 3);
	BOOST_CHECK_EQUAL (r.as_string(dcp::Standard::INTEROP), "03:06:08:003");
	BOOST_CHECK_EQUAL (r.as_string(dcp::Standard::SMPTE), "03:06:08:03");

	/* Another arbitrary tcr (30) */
	a = dcp::Time (24, 12, 6, 3, 30);
	b = dcp::Time (16, 8, 4, 2, 30);
	BOOST_CHECK_CLOSE (a / b, 1.5, 1e-5);

	a = dcp::Time (3600 * 24, 24, 250);
	BOOST_CHECK_EQUAL (a.h, 1);
	BOOST_CHECK_EQUAL (a.m, 0);
	BOOST_CHECK_EQUAL (a.s, 0);
	BOOST_CHECK_EQUAL (a.e, 0);

	a = dcp::Time (60 * 24, 24, 250);
	BOOST_CHECK_EQUAL (a.h, 0);
	BOOST_CHECK_EQUAL (a.m, 1);
	BOOST_CHECK_EQUAL (a.s, 0);
	BOOST_CHECK_EQUAL (a.e, 0);

	/* Check rounding; 3424 is 142.666666666... seconds or 0.166666666... ticks */
	a = dcp::Time (3424, 24, 250);
	BOOST_CHECK_EQUAL (a.h, 0);
	BOOST_CHECK_EQUAL (a.m, 2);
	BOOST_CHECK_EQUAL (a.s, 22);
	BOOST_CHECK_EQUAL (a.e, 167);

	a = dcp::Time (3425, 24, 250);
	BOOST_CHECK_EQUAL (a.h, 0);
	BOOST_CHECK_EQUAL (a.m, 2);
	BOOST_CHECK_EQUAL (a.s, 22);
	BOOST_CHECK_EQUAL (a.e, 177);

	/* Check addition of times with different tcrs */
	a = dcp::Time (0, 0, 0, 3, 24);
	b = dcp::Time (0, 0, 0, 4, 48);
	r = a + b;
	BOOST_CHECK_EQUAL (r, dcp::Time (0, 0, 0, 240, 1152));

	/* Check rounding on conversion from seconds */
	BOOST_CHECK_EQUAL (dcp::Time (80.990, 1000), dcp::Time (0, 1, 20, 990, 1000));

	/* Check rebase */
	a = dcp::Time (1, 58, 56, 2, 25);
	BOOST_CHECK_EQUAL (a.rebase(250), dcp::Time(1, 58, 56, 20, 250));
	b = dcp::Time (9, 12, 41, 17, 99);
	BOOST_CHECK_EQUAL (b.rebase(250), dcp::Time(9, 12, 41, 43, 250));
	a = dcp::Time (0, 2, 57, 999, 1000);
	BOOST_CHECK_EQUAL (a.rebase(250), dcp::Time(0, 2, 58, 0, 250));
	a = dcp::Time (0, 47, 9, 998, 1000);
	BOOST_CHECK_EQUAL (a.rebase(250), dcp::Time(0, 47, 10, 0, 250));

	/* Check some allowed constructions from string */

	/* Interop type 1 */
	a = dcp::Time ("01:23:45:123", optional<int>());
	BOOST_CHECK_EQUAL (a, dcp::Time (1, 23, 45, 123, 250));
	/* Interop type 2 */
	a = dcp::Time ("01:23:45.123", optional<int>());
	BOOST_CHECK_EQUAL (a, dcp::Time (1, 23, 45, 123, 1000));
	/* SMPTE */
	a = dcp::Time ("01:23:45:12", 250);
	BOOST_CHECK_EQUAL (a, dcp::Time (1, 23, 45, 12, 250));

	/* Check some disallowed constructions from string */
	BOOST_CHECK_THROW (dcp::Time ("01:23:45:1234", optional<int>()), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01:23:45:1234:66", optional<int>()), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01:23:45:", optional<int>()), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01:23::123", optional<int>()), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01::45:123", optional<int>()), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time (":23:45:123", optional<int>()), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01:23:45.1234", optional<int>()), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01:23:45.1234.66", optional<int>()), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01:23:45.", optional<int>()), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01:23:.123", optional<int>()), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01::45.123", optional<int>()), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time (":23:45.123", optional<int>()), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01:23:45:123", 250), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01:23:45:123:66", 250), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01:23:45:", 250), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01:23::123", 250), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time ("01::45:123", 250), dcp::ReadError);
	BOOST_CHECK_THROW (dcp::Time (":23:45:123", 250), dcp::ReadError);

	/* Check operator< and operator> */
	BOOST_CHECK (dcp::Time (3, 2, 3, 4, 24) < dcp::Time (3, 2, 3, 5, 24));
	BOOST_CHECK (!(dcp::Time (3, 2, 3, 4, 24) < dcp::Time (3, 2, 3, 4, 24)));
	BOOST_CHECK (dcp::Time (3, 2, 3, 5, 24) > dcp::Time (3, 2, 3, 4, 24));
	BOOST_CHECK (!(dcp::Time (3, 2, 3, 4, 24) > dcp::Time (3, 2, 3, 4, 24)));
	BOOST_CHECK (dcp::Time (6, 0, 0, 0, 24) < dcp::Time (7, 0, 0, 0, 24));
	BOOST_CHECK (dcp::Time (0, 6, 0, 0, 24) < dcp::Time (0, 7, 0, 0, 24));
	BOOST_CHECK (dcp::Time (0, 0, 6, 0, 24) < dcp::Time (0, 0, 7, 0, 24));
	BOOST_CHECK (dcp::Time (0, 0, 0, 6, 24) < dcp::Time (0, 0, 0, 7, 24));
	BOOST_CHECK (dcp::Time (7, 0, 0, 0, 24) > dcp::Time (6, 0, 0, 0, 24));
	BOOST_CHECK (dcp::Time (0, 7, 0, 0, 24) > dcp::Time (0, 6, 0, 0, 24));
	BOOST_CHECK (dcp::Time (0, 0, 7, 0, 24) > dcp::Time (0, 0, 6, 0, 24));
	BOOST_CHECK (dcp::Time (0, 0, 0, 7, 24) > dcp::Time (0, 0, 0, 6, 24));
}
