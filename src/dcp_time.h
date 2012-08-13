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

#ifndef LIBDCP_TIME_H
#define LIBDCP_TIME_H

namespace libdcp {

class Time
{
public:
	Time () : h (0), m (0), s (0), ms (0) {}
	Time (int frame, int frames_per_second);
	Time (int h_, int m_, int s_, int ms_)
		: h (h_)
		, m (m_)
		, s (s_)
		, ms (ms_)
	{}

	int h;
	int m;
	int s;
	int ms;
};

extern bool operator== (Time const & a, Time const & b);
extern bool operator<= (Time const & a, Time const & b);
extern std::ostream & operator<< (std::ostream & s, Time const & t);

}

#endif
