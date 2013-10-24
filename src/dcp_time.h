/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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
 *  @brief A representation of time within a DCP.
 */

#ifndef LIBDCP_TIME_H
#define LIBDCP_TIME_H

#include <string>
#include <iostream>
#include <stdint.h>

namespace libdcp {

/** @class Time
 *  @brief A representation of time within a DCP.
 */
	
class Time
{
public:
	Time () : h (0), m (0), s (0), t (0) {}

	Time (int64_t ticks);

	/** Construct a Time from a frame index (starting from 0)
	 *  and a frames per second count.
	 */
	Time (int frame, int frames_per_second);

	/** Construct a Time from hours, minutes, seconds and ticks.
	 *  @param h_ Hours.
	 *  @param m_ Minutes.
	 *  @param s_ Seconds.
	 *  @param t_ Ticks (where 1 tick is 4 milliseconds).
	 */
	Time (int h_, int m_, int s_, int t_)
		: h (h_)
		, m (m_)
		, s (s_)
		, t (t_)
	{}

	Time (std::string time);

	int h; ///< hours
	int m; ///< minutes
	int s; ///< seconds
	int t; ///< `ticks', where 1 tick is 4 milliseconds

	std::string to_string () const;
	int64_t to_ticks () const;

private:
	void set (double);
};

extern bool operator== (Time const & a, Time const & b);
extern bool operator!= (Time const & a, Time const & b);
extern bool operator<= (Time const & a, Time const & b);
extern bool operator< (Time const & a, Time const & b);
extern bool operator> (Time const & a, Time const & b);
extern std::ostream & operator<< (std::ostream & s, Time const & t);
extern Time operator+ (Time a, Time const & b);	
extern Time operator- (Time a, Time const & b);
extern float operator/ (Time a, Time const & b);

}

#endif
