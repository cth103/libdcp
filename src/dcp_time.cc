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

/** @file  src/dcp_time.cc
 *  @brief A representation of time within a DCP.
 */

#include <iostream>
#include <cmath>
#include "dcp_time.h"

using namespace std;
using namespace libdcp;

Time::Time (int frame, int frames_per_second)
	: h (0)
	, m (0)
	, s (0)
	, t (0)
{
	float sec_float = float (frame) / frames_per_second;
	t = (int (floor (sec_float * 1000)) % 1000) / 4;
	s = floor (sec_float);

	if (s > 60) {
		m = s / 60;
		s -= m * 60;
	}

	if (m > 60) {
		h = m / 60;
		m -= h * 60;
	}
}

bool
libdcp::operator== (Time const & a, Time const & b)
{
	return (a.h == b.h && a.m == b.m && a.s == b.s && a.t == b.t);
}

bool
libdcp::operator<= (Time const & a, Time const & b)
{
	if (a.h != b.h) {
		return a.h <= b.h;
	}

	if (a.m != b.m) {
		return a.m <= b.m;
	}

	if (a.s != b.s) {
		return a.s <= b.s;
	}

	if (a.t != b.t) {
		return a.t <= b.t;
	}

	return true;
}

ostream &
libdcp::operator<< (ostream& s, Time const & t)
{
	s << t.h << ":" << t.m << ":" << t.s << "." << t.t;
	return s;
}
