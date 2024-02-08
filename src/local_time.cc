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


LocalTime::LocalTime(boost::posix_time::ptime t, UTCOffset offset)
{
	set (t);
	_offset = offset;
}


/** Set our UTC offset to be according to the local time zone */
void
LocalTime::set_local_time_zone ()
{
	auto const utc_now = boost::posix_time::second_clock::universal_time ();
	auto const now = boost::date_time::c_local_adjustor<boost::posix_time::ptime>::utc_to_local (utc_now);
	auto offset = now - utc_now;

	_offset = { static_cast<int>(offset.hours()), static_cast<int>(offset.minutes()) };
}


LocalTime::LocalTime (string s)
{
	/* 2013-01-05T18:06:59[.frac][TZ]
	 * Where .frac is fractional seconds
	 *       TZ is something like +04:00
	 */

	if (s.length() < 19) {
		throw TimeFormatError (s);
	}

	/* Date and time with whole seconds */

	if (s[4] != '-' || s[7] != '-' || s[10] != 'T' || s[13] != ':' || s[16] != ':') {
		throw TimeFormatError(s);
	}

	_year = lexical_cast<int>(s.substr(0, 4));
	_month = lexical_cast<int>(s.substr(5, 2));
	_day = lexical_cast<int>(s.substr(8, 2));
	_hour = lexical_cast<int>(s.substr(11, 2));
	_minute = lexical_cast<int>(s.substr(14, 2));
	_second = lexical_cast<int>(s.substr(17, 2));

	size_t pos = 19;

	/* Fractional seconds */
	if (s.length() > pos && s[pos] == '.') {
		auto end = s.find('+', pos);
		if (end == std::string::npos) {
			end = s.find('-', pos);
		}
		if (end == std::string::npos) {
			end = s.length();
		}
		auto const length = end - pos;
		_millisecond = lexical_cast<int>(s.substr(pos + 1, std::min(static_cast<size_t>(3), length - 1)));
		pos = end;
	} else {
		_millisecond = 0;
	}

	/* Timezone */
	if (pos != s.length() && s[pos] != 'Z') {
		if (s[pos] != '+' && s[pos] != '-') {
			throw TimeFormatError(s);
		}
		if ((s.length() - pos) != 6) {
			throw TimeFormatError(s);
		}

		_offset.set_hour(lexical_cast<int>(s.substr(pos + 1, 2)));
		_offset.set_minute(lexical_cast<int>(s.substr(pos + 4, 2)));

		if (s[pos] == '-') {
			_offset.set_hour(-_offset.hour());
			_offset.set_minute(-_offset.minute());
		}
	}
}


string
LocalTime::as_string(bool with_millisecond, bool with_timezone) const
{
	char buffer[32];

	auto const written = snprintf(
		buffer, sizeof (buffer),
		"%sT%s",
		date().c_str(), time_of_day(true, with_millisecond).c_str()
		);

	DCP_ASSERT(written < 32);

	if (with_timezone) {
		snprintf(
			buffer + written, sizeof(buffer) - written,
			"%s%02d:%02d", (_offset.hour() >= 0 ? "+" : "-"), abs(_offset.hour()), abs(_offset.minute())
			);
	}
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
LocalTime::add(boost::posix_time::time_duration duration)
{
	using namespace boost;

	posix_time::ptime t(gregorian::date(_year, _month, _day), posix_time::time_duration(_hour, _minute, _second, _millisecond * 1000));
	t += duration;
	set (t);
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
	add(boost::posix_time::time_duration(0, m, 0));
}


bool
LocalTime::operator== (LocalTime const & other) const
{
	auto a = as_utc();
	auto b = other.as_utc();

	return a.year() == b.year() && a.month() == b.month() && a.day() == b.day() &&
		a.hour() == b.hour() && a.minute() == b.minute() && a.second() == b.second() && a.millisecond() == b.millisecond();
}


bool
LocalTime::operator< (LocalTime const & other) const
{
	auto a = as_utc();
	auto b = other.as_utc();

	if (a.year() != b.year()) {
		return a.year() < b.year();
	}
	if (a.month() != b.month()) {
		return a.month() < b.month();
	}
	if (a.day() != b.day()) {
		return a.day() < b.day();
	}
	if (a.hour() != b.hour()) {
		return a.hour() < b.hour();
	}
	if (a.minute() != b.minute()) {
		return a.minute() < other.minute();
	}
	if (a.second() != b.second()) {
		return a.second() < b.second();
	}
	return a.millisecond() < b.millisecond();
}


bool
LocalTime::operator<=(LocalTime const& other) const
{
	return *this < other || *this == other;
}



bool
LocalTime::operator>(LocalTime const & other) const
{
	auto a = as_utc();
	auto b = other.as_utc();

	if (a.year() != b.year()) {
		return a.year() > b.year();
	}
	if (a.month() != b.month()) {
		return a.month() > b.month();
	}
	if (a.day() != b.day()) {
		return a.day() > b.day();
	}
	if (a.hour() != b.hour()) {
		return a.hour() > b.hour();
	}
	if (a.minute() != b.minute()) {
		return a.minute() > b.minute();
	}
	if (a.second() != b.second()) {
		return a.second() > b.second();
	}
	return a.millisecond() > b.millisecond();
}


bool
LocalTime::operator>=(LocalTime const& other) const
{
	return *this > other || *this == other;
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


LocalTime
LocalTime::from_asn1_utc_time (string time)
{
	LocalTime t;
	sscanf(time.c_str(), "%2d%2d%2d%2d%2d%2d", &t._year, &t._month, &t._day, &t._hour, &t._minute, &t._second);

	if (t._year < 70) {
		t._year += 100;
	}
	t._year += 1900;

	t._millisecond = 0;
	t._offset = {};

	return t;
}


LocalTime
LocalTime::from_asn1_generalized_time (string time)
{
	LocalTime t;
	sscanf(time.c_str(), "%4d%2d%2d%2d%2d%2d", &t._year, &t._month, &t._day, &t._hour, &t._minute, &t._second);

	t._millisecond = 0;
	t._offset = {};

	return t;
}


LocalTime
LocalTime::as_utc() const
{
	auto t = *this;
	t.add(boost::posix_time::time_duration(-_offset.hour(), -_offset.minute(), 0));
	return t;
}

