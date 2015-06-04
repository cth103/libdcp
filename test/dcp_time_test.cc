/*
    Copyright (C) 2013-2015 Carl Hetherington <cth@carlh.net>

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

#include <boost/test/unit_test.hpp>
#include "dcp_time.h"

/** Check that dcp::Time works */
BOOST_AUTO_TEST_CASE (dcp_time)
{
	/* tcr of 250 makes the editable event length the same as an Interop `tick' */
	dcp::Time t (977143, 24, 250);

	BOOST_CHECK_EQUAL (t.e, 73);
	BOOST_CHECK_EQUAL (t.s, 34);
	BOOST_CHECK_EQUAL (t.m, 18);
	BOOST_CHECK_EQUAL (t.h, 11);
	BOOST_CHECK_EQUAL (t.as_string(), "11:18:34:073");

	/* Use a tcr of 24 so that the editable event is a frame */
	dcp::Time a (3, 2, 3, 4, 24);
	dcp::Time b (2, 3, 4, 5, 24);

	dcp::Time r = a - b;
	BOOST_CHECK_EQUAL (r.h, 0);
	BOOST_CHECK_EQUAL (r.m, 58);
	BOOST_CHECK_EQUAL (r.s, 58);
	BOOST_CHECK_EQUAL (r.e, 23);
	BOOST_CHECK_EQUAL (r.as_string(), "00:58:58:023");

	/* Different tcr (25) */
	a = dcp::Time (1, 58, 56, 2, 25);
	b = dcp::Time (1, 7, 12, 1, 25);
	r = a + b;
	BOOST_CHECK_EQUAL (r.h, 3);
	BOOST_CHECK_EQUAL (r.m, 6);
	BOOST_CHECK_EQUAL (r.s, 8);
	BOOST_CHECK_EQUAL (r.e, 3);
	BOOST_CHECK_EQUAL (r.as_string(), "03:06:08:003");

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

	/* Check rebase() */
	a = dcp::Time (1, 58, 56, 2, 25);
	BOOST_CHECK_EQUAL (a.rebase (250), dcp::Time (1, 58, 56, 20, 250));
	b = dcp::Time (9, 12, 41, 17, 99);
	BOOST_CHECK_EQUAL (b.rebase (250), dcp::Time (9, 12, 41, 43, 250));
}
