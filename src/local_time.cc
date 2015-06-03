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

/** @file  src/local_time.cc
 *  @brief LocalTime class.
 */

#include "local_time.h"
#include "exceptions.h"
#include <boost/lexical_cast.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <cstdio>

using std::string;
using boost::lexical_cast;
using namespace dcp;

/** Construct a LocalTime from the current time */
LocalTime::LocalTime ()
{
	time_t now = time (0);
	struct tm* tm = localtime (&now);

	_year = tm->tm_year + 1900;
	_month = tm->tm_mon + 1;
	_day = tm->tm_mday;
	_hour = tm->tm_hour;
	_minute = tm->tm_min;
	_second = tm->tm_sec;

	set_local_time_zone ();
}

/** Construct a LocalTime from a boost::posix_time::ptime using the local
 *  time zone.
 */
LocalTime::LocalTime (boost::posix_time::ptime t)
{
	_year = t.date().year ();
	_month = t.date().month ();
	_day = t.date().day ();
	_hour = t.time_of_day().hours ();
	_minute = t.time_of_day().minutes ();
	_second = t.time_of_day().seconds ();

	set_local_time_zone ();
}

/** Set our UTC offset to be according to the local time zone */
void
LocalTime::set_local_time_zone ()
{
	boost::posix_time::ptime const utc_now = boost::posix_time::second_clock::universal_time ();
	boost::posix_time::ptime const now = boost::date_time::c_local_adjustor<boost::posix_time::ptime>::utc_to_local (utc_now);
	boost::posix_time::time_duration offset = now - utc_now;

	_tz_hour = offset.hours ();
	_tz_minute = offset.minutes ();
}

/** @param s A string of the form 2013-01-05T18:06:59+04:00 */
LocalTime::LocalTime (string s)
{
	/* 2013-01-05T18:06:59+04:00 */
        /* 0123456789012345678901234 */
	
	if (s.length() < 25) {
		throw TimeFormatError (s);
	}

	/* Check incidental characters */
	if (s[4] != '-' || s[7] != '-' || s[10] != 'T' || s[13] != ':' || s[16] != ':' || s[22] != ':') {
		throw TimeFormatError (s);
	}
	
	_year = lexical_cast<int> (s.substr (0, 4));
	_month = lexical_cast<int> (s.substr (5, 2));
	_day = lexical_cast<int> (s.substr (8, 2));
	_hour = lexical_cast<int> (s.substr (11, 2));
	_minute = lexical_cast<int> (s.substr (14, 2));
	_second = lexical_cast<int> (s.substr (17, 2));
	_tz_hour = lexical_cast<int> (s.substr (20, 2));
	_tz_minute = lexical_cast<int> (s.substr (23, 2));

	if (s[19] == '-') {
		_tz_hour = -_tz_hour;
	} else if (s[19] != '+') {
		throw TimeFormatError (s);
	}
}

/** @return A string of the form 2013-01-05T18:06:59+04:00 */
string
LocalTime::as_string () const
{
	char buffer[32];
	snprintf (
		buffer, sizeof (buffer),
		"%sT%s%s%02d:%02d",
		date().c_str(), time_of_day().c_str(), (_tz_hour >= 0 ? "+" : "-"), abs (_tz_hour), _tz_minute
		);
	return buffer;
}

/** @return The date in the form YYYY-MM-DD */
string
LocalTime::date () const
{
	char buffer[32];
	snprintf (buffer, sizeof (buffer), "%04d-%02d-%02d", _year, _month, _day);
	return buffer;
}

/** @return The time in the form HH:MM:SS */
string
LocalTime::time_of_day () const
{
	char buffer[32];
	snprintf (buffer, sizeof (buffer), "%02d:%02d:%02d", _hour, _minute, _second);
	return buffer;
}
