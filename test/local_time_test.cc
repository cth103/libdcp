/*
    Copyright (C) 2014-2021 Carl Hetherington <cth@carlh.net>

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


#include "exceptions.h"
#include "local_time.h"
#include <boost/test/unit_test.hpp>


/** Check that dcp::LocalTime works */
BOOST_AUTO_TEST_CASE (local_time_basic_test)
{
	/* Badly-formatted times */
	BOOST_CHECK_THROW (dcp::LocalTime (""), dcp::TimeFormatError);
	BOOST_CHECK_THROW (dcp::LocalTime ("XXX"), dcp::TimeFormatError);
	BOOST_CHECK_THROW (dcp::LocalTime ("2013-01-05T18:06:59+04:0"), dcp::TimeFormatError);
	BOOST_CHECK_THROW (dcp::LocalTime ("2013-01-05T18:06:59X04:00"), dcp::TimeFormatError);
	BOOST_CHECK_THROW (dcp::LocalTime ("2013-01-05T18-06:59+04:00"), dcp::TimeFormatError);
	BOOST_CHECK_THROW (dcp::LocalTime ("2013!01-05T18:06:59+04:00"), dcp::TimeFormatError);

	/* Correctly-formatted */

	{
		dcp::LocalTime t("2013-01-05T18:06:59");
		BOOST_CHECK_EQUAL(t._year, 2013);
		BOOST_CHECK_EQUAL(t._month, 1);
		BOOST_CHECK_EQUAL(t._day, 5);
		BOOST_CHECK_EQUAL(t._hour, 18);
		BOOST_CHECK_EQUAL(t._minute, 6);
		BOOST_CHECK_EQUAL(t._second, 59);
		BOOST_CHECK(t._offset == dcp::UTCOffset(0, 0));
		BOOST_CHECK_EQUAL(t.as_string(), "2013-01-05T18:06:59+00:00");
	}

	{
		dcp::LocalTime t ("2013-01-05T18:06:59+04:00");
		BOOST_CHECK_EQUAL (t._year, 2013);
		BOOST_CHECK_EQUAL (t._month, 1);
		BOOST_CHECK_EQUAL (t._day, 5);
		BOOST_CHECK_EQUAL (t._hour, 18);
		BOOST_CHECK_EQUAL (t._minute, 6);
		BOOST_CHECK_EQUAL (t._second, 59);
		BOOST_CHECK(t._offset == dcp::UTCOffset(4, 0));
		BOOST_CHECK_EQUAL (t.as_string(), "2013-01-05T18:06:59+04:00");
	}

	{
		dcp::LocalTime t ("2011-11-20T01:06:59-09:30");
		BOOST_CHECK_EQUAL (t._year, 2011);
		BOOST_CHECK_EQUAL (t._month, 11);
		BOOST_CHECK_EQUAL (t._day, 20);
		BOOST_CHECK_EQUAL (t._hour, 1);
		BOOST_CHECK_EQUAL (t._minute, 6);
		BOOST_CHECK_EQUAL (t._second, 59);
		BOOST_CHECK(t._offset == dcp::UTCOffset(-9, -30));
		BOOST_CHECK_EQUAL (t.as_string(), "2011-11-20T01:06:59-09:30");
	}

	{
		dcp::LocalTime t ("2011-11-20T01:06:59.456-09:30");
		BOOST_CHECK_EQUAL (t._year, 2011);
		BOOST_CHECK_EQUAL (t._month, 11);
		BOOST_CHECK_EQUAL (t._day, 20);
		BOOST_CHECK_EQUAL (t._hour, 1);
		BOOST_CHECK_EQUAL (t._minute, 6);
		BOOST_CHECK_EQUAL (t._second, 59);
		BOOST_CHECK_EQUAL (t._millisecond, 456);
		BOOST_CHECK(t._offset == dcp::UTCOffset(-9, -30));
		BOOST_CHECK_EQUAL (t.as_string(true), "2011-11-20T01:06:59.456-09:30");
	}

	{
		dcp::LocalTime t ("2011-11-20T01:06:59.456-09:30");
		BOOST_CHECK_EQUAL (t._year, 2011);
		BOOST_CHECK_EQUAL (t._month, 11);
		BOOST_CHECK_EQUAL (t._day, 20);
		BOOST_CHECK_EQUAL (t._hour, 1);
		BOOST_CHECK_EQUAL (t._minute, 6);
		BOOST_CHECK_EQUAL (t._second, 59);
		BOOST_CHECK_EQUAL (t._millisecond, 456);
		BOOST_CHECK(t._offset == dcp::UTCOffset(-9, -30));
		BOOST_CHECK_EQUAL (t.as_string(true, false), "2011-11-20T01:06:59.456");
	}

	{
		dcp::LocalTime t ("2011-11-20T01:06:59.456-09:30");
		BOOST_CHECK_EQUAL (t._year, 2011);
		BOOST_CHECK_EQUAL (t._month, 11);
		BOOST_CHECK_EQUAL (t._day, 20);
		BOOST_CHECK_EQUAL (t._hour, 1);
		BOOST_CHECK_EQUAL (t._minute, 6);
		BOOST_CHECK_EQUAL (t._second, 59);
		BOOST_CHECK_EQUAL (t._millisecond, 456);
		BOOST_CHECK(t._offset == dcp::UTCOffset(-9, -30));
		BOOST_CHECK_EQUAL (t.as_string(false, false), "2011-11-20T01:06:59");
	}

	{
		dcp::LocalTime t("2011-11-20T01:06:59.45678901-09:30");
		BOOST_CHECK_EQUAL(t._year, 2011);
		BOOST_CHECK_EQUAL(t._month, 11);
		BOOST_CHECK_EQUAL(t._day, 20);
		BOOST_CHECK_EQUAL(t._hour, 1);
		BOOST_CHECK_EQUAL(t._minute, 6);
		BOOST_CHECK_EQUAL(t._second, 59);
		/* The fractional seconds here is truncated rather than rounded, for better or worse */
		BOOST_CHECK_EQUAL(t._millisecond, 456);
		BOOST_CHECK(t._offset == dcp::UTCOffset(-9, -30));
		BOOST_CHECK_EQUAL(t.as_string(false, false), "2011-11-20T01:06:59");
	}

	{
		dcp::LocalTime t("2024-01-23T23:21:32Z");
		BOOST_CHECK_EQUAL(t._year, 2024);
		BOOST_CHECK_EQUAL(t._month, 1);
		BOOST_CHECK_EQUAL(t._day, 23);
		BOOST_CHECK_EQUAL(t._hour, 23);
		BOOST_CHECK_EQUAL(t._minute, 21);
		BOOST_CHECK_EQUAL(t._second, 32);
		BOOST_CHECK_EQUAL(t._millisecond, 0);
		BOOST_CHECK(t._offset == dcp::UTCOffset(0, 0));
		BOOST_CHECK_EQUAL(t.as_string(false, false), "2024-01-23T23:21:32");
	}

	{
		/* Construction from boost::posix_time::ptime */
		dcp::LocalTime b (boost::posix_time::time_from_string ("2002-01-20 19:03:56"));
		BOOST_CHECK_EQUAL (b._year, 2002);
		BOOST_CHECK_EQUAL (b._month, 1);
		BOOST_CHECK_EQUAL (b._day, 20);
		BOOST_CHECK_EQUAL (b._hour, 19);
		BOOST_CHECK_EQUAL (b._minute, 3);
		BOOST_CHECK_EQUAL (b._second, 56);
	}

	{
		/* Construction from boost::posix_time::ptime with milliseconds */
		dcp::LocalTime b (boost::posix_time::time_from_string ("2002-01-20 19:03:56.491"));
		BOOST_CHECK_EQUAL (b._year, 2002);
		BOOST_CHECK_EQUAL (b._month, 1);
		BOOST_CHECK_EQUAL (b._day, 20);
		BOOST_CHECK_EQUAL (b._hour, 19);
		BOOST_CHECK_EQUAL (b._minute, 3);
		BOOST_CHECK_EQUAL (b._second, 56);
		BOOST_CHECK_EQUAL (b._millisecond, 491);
	}

	{
		dcp::LocalTime b ("2015-11-18T19:26:45");
		BOOST_CHECK_EQUAL (b._year, 2015);
		BOOST_CHECK_EQUAL (b._month, 11);
		BOOST_CHECK_EQUAL (b._day, 18);
		BOOST_CHECK_EQUAL (b._hour, 19);
		BOOST_CHECK_EQUAL (b._minute, 26);
		BOOST_CHECK_EQUAL (b._second, 45);
		BOOST_CHECK_EQUAL (b._millisecond, 0);
		BOOST_CHECK(b._offset == dcp::UTCOffset());
	}

	/* Check negative times with non-zero timezone offset minutes */
	{
		dcp::LocalTime t ("2013-01-05T18:06:59-04:30");
		BOOST_CHECK_EQUAL (t._year, 2013);
		BOOST_CHECK_EQUAL (t._month, 1);
		BOOST_CHECK_EQUAL (t._day, 5);
		BOOST_CHECK_EQUAL (t._hour, 18);
		BOOST_CHECK_EQUAL (t._minute, 6);
		BOOST_CHECK_EQUAL (t._second, 59);
		BOOST_CHECK(t._offset == dcp::UTCOffset(-4, -30));
		BOOST_CHECK_EQUAL (t.as_string(), "2013-01-05T18:06:59-04:30");
	}

	/* KDM seen with a TZ offset of -07:59, which we used to reject because
	 * we only accepted minutes between -30 and +30 (for some reason that I
	 * now can't find).
	 */
	{
		dcp::LocalTime t("2023-11-30T23:59:00-07:59");
		BOOST_CHECK_EQUAL(t._year, 2023);
		BOOST_CHECK_EQUAL(t._month, 11);
		BOOST_CHECK_EQUAL(t._day, 30);
		BOOST_CHECK_EQUAL(t._hour, 23);
		BOOST_CHECK_EQUAL(t._minute, 59);
		BOOST_CHECK_EQUAL(t._second, 00);
		BOOST_CHECK(t._offset == dcp::UTCOffset(-7, -59));
		BOOST_CHECK_EQUAL(t.as_string(), "2023-11-30T23:59:00-07:59");
	}
}


BOOST_AUTO_TEST_CASE (local_time_add_minutes_test)
{
	{
		dcp::LocalTime t("2018-01-01T10:00:00+01:00");
		t.add_minutes (3);
		BOOST_CHECK_EQUAL (t.as_string(), "2018-01-01T10:03:00+01:00");
	}

	{
		dcp::LocalTime t("2018-01-01T10:00:15+01:00");
		t.add_minutes (3);
		BOOST_CHECK_EQUAL (t.as_string(), "2018-01-01T10:03:15+01:00");
	}

	{
		dcp::LocalTime t("2018-01-01T10:40:20+01:00");
		t.add_minutes (23);
		BOOST_CHECK_EQUAL (t.as_string(), "2018-01-01T11:03:20+01:00");
	}

	{
		dcp::LocalTime t("2018-01-01T10:40:20+01:00");
		t.add_minutes (123);
		BOOST_CHECK_EQUAL (t.as_string(), "2018-01-01T12:43:20+01:00");
	}

	{
		dcp::LocalTime t("2018-01-01T23:55:00+01:00");
		t.add_minutes (7);
		BOOST_CHECK_EQUAL (t.as_string(), "2018-01-02T00:02:00+01:00");
	}

	{
		dcp::LocalTime t("2018-01-31T23:55:00+01:00");
		t.add_minutes (7);
		BOOST_CHECK_EQUAL (t.as_string(), "2018-02-01T00:02:00+01:00");
	}

	{
		dcp::LocalTime t("2018-01-31T23:55:00.123");
		t.add_minutes (7);
		BOOST_CHECK_EQUAL (t, dcp::LocalTime("2018-02-01T00:02:00.123"));
	}
}


BOOST_AUTO_TEST_CASE (local_time_add_months_test)
{
	{
		dcp::LocalTime t("2013-06-23T18:06:59.123");
		t.add_months(-1);
		BOOST_CHECK_EQUAL (t, dcp::LocalTime("2013-05-23T18:06:59.123"));
		t.add_months(1);
		BOOST_CHECK_EQUAL (t, dcp::LocalTime("2013-06-23T18:06:59.123"));
		t.add_months(1);
		BOOST_CHECK_EQUAL (t, dcp::LocalTime("2013-07-23T18:06:59.123"));
		t.add_months(4);
		BOOST_CHECK_EQUAL (t, dcp::LocalTime("2013-11-23T18:06:59.123"));
		t.add_months(2);
		BOOST_CHECK_EQUAL (t, dcp::LocalTime("2014-01-23T18:06:59.123"));
		t.add_months(-14);
		BOOST_CHECK_EQUAL (t, dcp::LocalTime("2012-11-23T18:06:59.123"));
		t.add_months(14);
		BOOST_CHECK_EQUAL (t, dcp::LocalTime("2014-01-23T18:06:59.123"));
	}

	{
		dcp::LocalTime t("2018-01-30T11:00:00+01:00");
		t.add_months (1);
		BOOST_CHECK_EQUAL (t.as_string(), "2018-02-28T11:00:00+01:00");
	}
}


BOOST_AUTO_TEST_CASE (local_time_from_asn1_utctime_test)
{
	BOOST_CHECK_EQUAL (dcp::LocalTime::from_asn1_utc_time("991231235952").as_string(), "1999-12-31T23:59:52+00:00");
	BOOST_CHECK_EQUAL (dcp::LocalTime::from_asn1_utc_time("210215165952").as_string(), "2021-02-15T16:59:52+00:00");
}


BOOST_AUTO_TEST_CASE (local_time_from_asn1_generalized_time_test)
{
	BOOST_CHECK_EQUAL (dcp::LocalTime::from_asn1_generalized_time("19991231235952").as_string(), "1999-12-31T23:59:52+00:00");
	BOOST_CHECK_EQUAL (dcp::LocalTime::from_asn1_generalized_time("20210215165952").as_string(), "2021-02-15T16:59:52+00:00");
}


BOOST_AUTO_TEST_CASE(local_time_comparison_test)
{
	BOOST_CHECK(dcp::LocalTime("2014-01-01T10:00:00") < dcp::LocalTime("2014-01-01T10:05:00"));
	BOOST_CHECK(dcp::LocalTime("2014-01-01T10:00:00") < dcp::LocalTime("2015-01-01T10:00:00"));
	BOOST_CHECK(dcp::LocalTime("2014-01-01T10:00:00") < dcp::LocalTime("2014-01-01T11:00:00"));
	BOOST_CHECK(dcp::LocalTime("2014-10-10T10:00:00") < dcp::LocalTime("2014-10-10T10:00:01"));
	BOOST_CHECK(!(dcp::LocalTime("2014-10-10T10:00:00") < dcp::LocalTime("2014-10-10T10:00:00")));
	BOOST_CHECK(dcp::LocalTime("2014-10-10T10:00:00+01:00") < dcp::LocalTime("2014-10-10T10:00:00"));
	BOOST_CHECK(dcp::LocalTime("2014-10-10T10:00:00+01:30") < dcp::LocalTime("2014-10-10T10:00:00"));
	BOOST_CHECK(dcp::LocalTime("2014-10-10T10:00:00+01:00") < dcp::LocalTime("2014-10-10T10:00:01+01:00"));
	BOOST_CHECK(dcp::LocalTime("2014-01-01T10:00:00") < dcp::LocalTime("2014-01-01T10:05:00"));
	BOOST_CHECK(dcp::LocalTime("2014-10-10T10:00:00") < dcp::LocalTime("2014-10-10T10:00:00-01:30"));

	BOOST_CHECK(dcp::LocalTime("2014-01-01T10:05:00") > dcp::LocalTime("2014-01-01T10:00:00"));
	BOOST_CHECK(dcp::LocalTime("2014-10-10T10:00:00-01:30") > dcp::LocalTime("2014-10-10T10:00:00"));
	BOOST_CHECK(dcp::LocalTime("2014-01-01T10:05:00") > dcp::LocalTime("2014-01-01T10:00:00"));
	BOOST_CHECK(dcp::LocalTime("2015-01-01T10:00:00") > dcp::LocalTime("2014-01-01T10:00:00"));
	BOOST_CHECK(dcp::LocalTime("2014-01-01T11:00:00") > dcp::LocalTime("2014-01-01T10:00:00"));
	BOOST_CHECK(dcp::LocalTime("2014-10-10T10:00:01") > dcp::LocalTime("2014-10-10T10:00:00"));
	BOOST_CHECK(!(dcp::LocalTime("2014-10-10T10:00:00") > dcp::LocalTime("2014-10-10T10:00:00")));
	BOOST_CHECK(dcp::LocalTime("2014-10-10T10:00:00") > dcp::LocalTime("2014-10-10T10:00:00+01:00"));
	BOOST_CHECK(dcp::LocalTime("2014-10-10T10:00:00") > dcp::LocalTime("2014-10-10T10:00:00+01:30"));
	BOOST_CHECK(dcp::LocalTime("2014-10-10T10:00:01+01:00") > dcp::LocalTime("2014-10-10T10:00:00+01:00"));

	BOOST_CHECK(dcp::LocalTime("2014-01-01T10:00:00") != dcp::LocalTime("2014-01-01T10:05:00"));
	BOOST_CHECK(dcp::LocalTime("2014-01-01T10:00:00") == dcp::LocalTime("2014-01-01T10:00:00"));
	BOOST_CHECK(dcp::LocalTime("2014-01-01T10:00:00+02:00") == dcp::LocalTime("2014-01-01T08:00:00"));
	BOOST_CHECK(dcp::LocalTime("2014-01-01T10:00:00+02:00") == dcp::LocalTime("2014-01-01T11:00:00+03:00"));
}
