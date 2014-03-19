/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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
#include "local_time.h"
#include "exceptions.h"

/** Check that dcp::LocalTime works */
BOOST_AUTO_TEST_CASE (local_time_test)
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
		dcp::LocalTime t ("2013-01-05T18:06:59+04:00");
		BOOST_CHECK_EQUAL (t._year, 2013);
		BOOST_CHECK_EQUAL (t._month, 1);
		BOOST_CHECK_EQUAL (t._day, 5);
		BOOST_CHECK_EQUAL (t._hour, 18);
		BOOST_CHECK_EQUAL (t._minute, 6);
		BOOST_CHECK_EQUAL (t._second, 59);
		BOOST_CHECK_EQUAL (t._tz_hour, 4);
		BOOST_CHECK_EQUAL (t._tz_minute, 0);
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
		BOOST_CHECK_EQUAL (t._tz_hour, -9);
		BOOST_CHECK_EQUAL (t._tz_minute, 30);
		BOOST_CHECK_EQUAL (t.as_string(), "2011-11-20T01:06:59-09:30");
	}	
}

