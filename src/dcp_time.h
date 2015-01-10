/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

/** @file  src/dcp_time.h
 *  @brief Time class.
 */

#ifndef LIBDCP_TIME_H
#define LIBDCP_TIME_H

#include <stdint.h>
#include <string>
#include <iostream>

namespace dcp {

/** @class Time
 *  @brief A representation of time within a DCP.
 */
	
class Time
{
public:
	Time () : h (0), m (0), s (0), e (0), tcr (1) {}

	/** Construct a Time from a frame index (starting from 0),
	 *  a frames per second count and a timecode rate.
	 */
	Time (int frame, int frames_per_second, int tcr);

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

	Time (std::string time, int tcr);

	int h; ///<   hours
	int m; ///<   minutes
	int s; ///<   seconds
	int e; ///<   editable units (where 1 editable unit is 1 / tcr_ seconds)
	int tcr; ///< timecode rate: the number of editable units per second.

	std::string to_string () const;
	int64_t to_editable_units (int tcr_) const;

private:
	void set (double seconds, int tcr);
};

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

}

#endif
