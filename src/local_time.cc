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


/** @file  src/local_time.cc
 *  @brief LocalTime class
 */


#include "local_time.h"
#include "exceptions.h"
#include "dcp_assert.h"
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <cstdio>


using std::string;
using std::ostream;
using boost::lexical_cast;
using namespace dcp;


LocalTime::LocalTime ()
{
	auto now = time (0);
	auto tm = localtime (&now);
	set (tm);
	set_local_time_zone ();
}


LocalTime::LocalTime (struct tm t)
{
	set (&t);
	set_local_time_zone ();
}


void
LocalTime::set (struct tm const * tm)
{
	_year = tm->tm_year + 1900;
	_month = tm->tm_mon + 1;
	_day = tm->tm_mday;
	_hour = tm->tm_hour;
	_minute = tm->tm_min;
	_second = tm->tm_sec;
	_millisecond = 0;
}


LocalTime::LocalTime (boost::posix_time::ptime t)
{
	set (t);
	set_local_time_zone ();
}


void
LocalTime::set (boost::posix_time::ptime t)
{
	_year = t.date().year ();
	_month = t.date().month ();
	_day = t.date().day ();
	_hour = t.time_of_day().hours ();
	_minute = t.time_of_day().minutes ();
	_second = t.time_of_day().seconds ();
	_millisecond = t.time_of_day().fractional_seconds () / 1000;
	DCP_ASSERT (_millisecond < 1000);
}


LocalTime::LocalTime (boost::posix_time::ptime t, int tz_hour, int tz_minute)
{
	set (t);
	_tz_hour = tz_hour;
	_tz_minute = tz_minute;
}


/** Set our UTC offset to be according to the local time zone */
void
LocalTime::set_local_time_zone ()
{
	auto const utc_now = boost::posix_time::second_clock::universal_time ();
	auto const now = boost::date_time::c_local_adjustor<boost::posix_time::ptime>::utc_to_local (utc_now);
	auto offset = now - utc_now;

	_tz_hour = offset.hours ();
	_tz_minute = offset.minutes ();
}


LocalTime::LocalTime (string s)
{
	/* 2013-01-05T18:06:59 or 2013-01-05T18:06:59.123 or 2013-01-05T18:06:59+04:00 or 2013-01-05T18:06:59.123+04:00 */
	/* 0123456789012345678 or 01234567890123456789012 or 0123456789012345678901234 or 01234567890123456789012345678 */

	if (s.length() < 19) {
		throw TimeFormatError (s);
	}

	bool with_millisecond = false;
	bool with_tz = false;

	switch (s.length ()) {
	case 19:
		break;
	case 23:
		with_millisecond = true;
		break;
	case 25:
		with_tz = true;
		break;
	case 29:
		with_millisecond = with_tz = true;
		break;
	default:
		throw TimeFormatError (s);
	}

	int const tz_pos = with_millisecond ? 23 : 19;

	/* Check incidental characters */
	if (s[4] != '-' || s[7] != '-' || s[10] != 'T' || s[13] != ':' || s[16] != ':') {
		throw TimeFormatError (s);
	}
	if (with_millisecond && s[19] != '.') {
		throw TimeFormatError (s);
	}
	if (with_tz && s[tz_pos] != '+' && s[tz_pos] != '-') {
		throw TimeFormatError (s);
	}

	_year = lexical_cast<int>(s.substr(0, 4));
	_month = lexical_cast<int>(s.substr(5, 2));
	_day = lexical_cast<int>(s.substr(8, 2));
	_hour = lexical_cast<int>(s.substr(11, 2));
	_minute = lexical_cast<int>(s.substr(14, 2));
	_second = lexical_cast<int>(s.substr(17, 2));
	_millisecond = with_millisecond ? lexical_cast<int>(s.substr(20, 3)) : 0;
	_tz_hour = with_tz ? lexical_cast<int>(s.substr(tz_pos + 1, 2)) : 0;
	_tz_minute = with_tz ? lexical_cast<int>(s.substr(tz_pos + 4, 2)) : 0;

	if (with_tz && s[tz_pos] == '-') {
		_tz_hour = -_tz_hour;
		_tz_minute = -_tz_minute;
	}
}


string
LocalTime::as_string (bool with_millisecond) const
{
	char buffer[32];
	snprintf (
		buffer, sizeof (buffer),
		"%sT%s%s%02d:%02d",
		date().c_str(), time_of_day(true, with_millisecond).c_str(), (_tz_hour >= 0 ? "+" : "-"), abs (_tz_hour), abs(_tz_minute)
		);
	return buffer;
}


string
LocalTime::date () const
{
	char buffer[32];
	snprintf (buffer, sizeof (buffer), "%04d-%02d-%02d", _year, _month, _day);
	return buffer;
}


string
LocalTime::time_of_day (bool with_second, bool with_millisecond) const
{
	char buffer[32];
	DCP_ASSERT(!(with_millisecond && !with_second));
	if (with_millisecond) {
		snprintf (buffer, sizeof (buffer), "%02d:%02d:%02d.%03d", _hour, _minute, _second, _millisecond);
	} else if (with_second) {
		snprintf (buffer, sizeof (buffer), "%02d:%02d:%02d", _hour, _minute, _second);
	} else {
		snprintf (buffer, sizeof (buffer), "%02d:%02d", _hour, _minute);
	}
	return buffer;
}


void
LocalTime::add_days (int days)
{
	using namespace boost;

	gregorian::date d (_year, _month, _day);
	if (days > 0) {
		d += gregorian::days (days);
	} else {
		d -= gregorian::days (-days);
	}

	set (posix_time::ptime(d, posix_time::time_duration(_hour, _minute, _second, _millisecond * 1000)));
}


void
LocalTime::add_months (int m)
{
	using namespace boost;

	gregorian::date d (_year, _month, _day);
	if (m > 0) {
		d += gregorian::months (m);
	} else {
		d -= gregorian::months (-m);
	}

	set (posix_time::ptime(d, posix_time::time_duration(_hour, _minute, _second, _millisecond * 1000)));
}


void
LocalTime::add_minutes (int m)
{
	using namespace boost;

	posix_time::ptime t(gregorian::date(_year, _month, _day), posix_time::time_duration(_hour, _minute, _second, _millisecond * 1000));
	t += posix_time::time_duration(0, m, 0);
	set (t);
}


bool
LocalTime::operator== (LocalTime const & other) const
{
	return _year == other._year && _month == other._month && _day == other._day &&
		_hour == other._hour && _second == other._second && _millisecond == other._millisecond &&
		_tz_hour == other._tz_hour && _tz_minute == other._tz_minute;
}


bool
LocalTime::operator< (LocalTime const & other) const
{
	if (_year != other._year) {
		return _year < other._year;
	}
	if (_month != other._month) {
		return _month < other._month;
	}
	if (_day != other._day) {
		return _day < other._day;
	}
	if (_hour != other._hour) {
		return _hour < other._hour;
	}
	if (_second != other._second) {
		return _second < other._second;
	}
	return _millisecond < other._millisecond;
}


bool
LocalTime::operator!= (LocalTime const & other) const
{
	return !(*this == other);
}


ostream&
dcp::operator<< (ostream& s, LocalTime const & t)
{
	s << t.as_string ();
	return s;
}
