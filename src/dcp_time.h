/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/dcp_time.h
 *  @brief Time class
 */


#ifndef LIBDCP_TIME_H
#define LIBDCP_TIME_H


#include "types.h"
#include <boost/optional.hpp>
#include <stdint.h>
#include <string>
#include <iostream>


namespace dcp {


class Time;


extern bool operator== (Time const & a, Time const & b);
extern bool operator!= (Time const & a, Time const & b);
extern bool operator<= (Time const & a, Time const & b);
extern bool operator< (Time const & a, Time const & b);
extern bool operator> (Time const & a, Time const & b);
extern bool operator>= (Time const & a, Time const & b);
extern std::ostream & operator<< (std::ostream & s, Time const & t);
extern Time operator+ (Time a, Time b);
extern Time operator- (Time a, Time b);
extern float operator/ (Time a, Time const & b);


/** @class Time
 *  @brief A representation of time within a DCP.
 */
class Time
{
public:
	/** Construct a zero Time */
	Time () {}

	/** Construct a Time.
	 *  @param frame Frame index (starting from 0).
	 *  @param frames_per_second Frames per second.
	 *  @param tcr Timecode rate.
	 */
	Time (int frame, double frames_per_second, int tcr);

	/** Construct a Time from hours, minutes, seconds, editable units and a timecode rate.
	 *  @param h_ Hours.
	 *  @param m_ Minutes.
	 *  @param s_ Seconds.
	 *  @param e_ Editable units (where 1 editable unit is 1 / tcr_ seconds)
	 *  @param tcr_ Timecode rate; i.e. number of editable units per second.
	 */
	Time (int h_, int m_, int s_, int e_, int tcr_)
		: h (h_)
		, m (m_)
		, s (s_)
		, e (e_)
		, tcr (tcr_)
	{}

	/** Construct a Time from a number of seconds and a timecode rate
	 *
	 *  @param seconds A number of seconds
	 *  @param tcr_ Timecode rate
	 */
	Time (double seconds, int tcr);

	/** @param time String of the form
	 *     HH:MM:SS:EE                          for SMPTE
	 *     HH:MM:SS:E[E[E]] or HH:MM:SS.s[s[s]] for Interop
	 *  where HH are hours, MM minutes, SS seconds, EE editable units and
	 *  sss millseconds.
	 *
	 *  @param tcr_ Timecode rate if this is a SMPTE time, otherwise empty for an Interop time
	 */
	Time (std::string time, boost::optional<int> tcr);

	int h = 0; ///<   hours
	int m = 0; ///<   minutes
	int s = 0; ///<   seconds
	int e = 0; ///<   editable units (where 1 editable unit is 1 / tcr_ seconds)
	int tcr = 1; ///< timecode rate: the number of editable units per second.

	/** @return A string of the form h:m:s:e padded as in 00:00:00:000 (for Interop) or 00:00:00:00 (for SMPTE) */
	std::string as_string (Standard standard) const;

	/** @return the total number of seconds that this time consists of */
	double as_seconds () const;

	/** @param tcr_ Timecode rate with which the return value should be counted
	 *  @return the total number of editable units that this time consists of at the specified timecode rate, rounded down
	 *  to the nearest editable unit. For example, as_editable_units_floor(24) returns the total time in frames at 24fps.
	 */
	int64_t as_editable_units_floor (int tcr_) const;

	/** @param tcr_ Timecode rate with which the return value should be counted
	 *  @return the total number of editable units that this time consists of at the specified timecode rate, rounded up
	 *  to the nearest editable unit. For example, as_editable_units_ceil(24) returns the total time in frames at 24fps.
	 */
	int64_t as_editable_units_ceil (int tcr_) const;

	/** @param tcr_ New timecode rate
	 *  @return A new Time which is this time at the spcified new timecode rate
	 */
	Time rebase (int tcr_) const;

	Time& operator+= (Time const & o) {
		*this = *this + o;
		return *this;
	}

	Time& operator-= (Time const & o) {
		*this = *this - o;
		return *this;
	}

private:
	void set (double seconds, int tcr);
};


}


#endif
