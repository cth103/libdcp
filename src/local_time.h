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


/** @file  src/local_time.h
 *  @brief LocalTime class
 */


#ifndef LIBDCP_LOCAL_TIME_H
#define LIBDCP_LOCAL_TIME_H


#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>


class local_time_basic_test;


namespace dcp {


/** @class LocalTime
 *  @brief A representation of a local time (down to the second), including its offset
 *  from GMT (equivalent to xs:dateTime).
 *
 *  I tried to use boost for this, really I did, but I could not get it
 *  to parse strings of the required format (those that include time zones).
 *
 *  See http://www.w3.org/TR/xmlschema-2/#dateTime
 */
class LocalTime
{
public:
	/** Construct a LocalTime from the current time */
	LocalTime ();

	explicit LocalTime (struct tm tm);

	/** Construct a LocalTime from a boost::posix_time::ptime using the local
	 *  time zone
	 */
	explicit LocalTime (boost::posix_time::ptime);

	/** Construct a LocalTime from a boost::posix_time::ptime and a time zone offset
	 *  @param tz_minute Offset from UTC in minutes; if the timezone is behind UTC this may be negative,
	 *  e.g. -04:30 would have tz_hour=-1 and tz_minute=-30.
	 */
	LocalTime (boost::posix_time::ptime, int tz_hour, int tz_minute);

	/** @param s A string of the form 2013-01-05T18:06:59[.123][+04:00] */
	explicit LocalTime (std::string s);

	/** @return A string of the form 2013-01-05T18:06:59+04:00 or 2013-01-05T18:06:59.123+04:00 */
	std::string as_string (bool with_millisecond = false) const;

	/** @return The date in the form YYYY-MM-DD */
	std::string date () const;

	/** @return The time in the form HH:MM:SS or HH:MM:SS.mmm */
	std::string time_of_day (bool with_second, bool with_millisecond) const;

	int day () const {
		return _day;
	}

	int month () const {
		return _month;
	}

	int year () const {
		return _year;
	}

	void set_year (int y) {
		_year = y;
	}

	void add_days (int d);
	void add_months (int a);
	void add_minutes (int a);

	bool operator== (LocalTime const & other) const;
	bool operator!= (LocalTime const & other) const;
	bool operator< (LocalTime const & other) const;

private:
	friend class ::local_time_basic_test;

	void set (struct tm const * tm);
	void set (boost::posix_time::ptime);
	void set_local_time_zone ();

	/* Local time */
	int _year = 0;        ///< year
	int _month = 0;       ///< month number of the year (1-12)
	int _day = 0;         ///< day number of the month (1-31)
	int _hour = 0;        ///< hour number of the day (0-23)
	int _minute = 0;      ///< minute number of the hour (0-59)
	int _second = 0;      ///< second number of the minute (0-59)
	int _millisecond = 0; ///< millisecond number of the second (0-999)

	int _tz_hour = 0;     ///< hours by which this time is offset from UTC; can be negative
	/** Minutes by which this time is offset from UTC; if _tz_hour is negative
	 *  this will be either 0 or negative.
	 */
	int _tz_minute = 0;
};


std::ostream&
operator<< (std::ostream& s, LocalTime const & t);


}


#endif
