/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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
 *  @brief Time class.
 */

#include "raw_convert.h"
#include "dcp_time.h"
#include "exceptions.h"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <vector>
#include <cmath>

using namespace std;
using namespace boost;
using namespace dcp;

Time::Time (int frame, int frames_per_second)
	: h (0)
	, m (0)
	, s (0)
	, t (0)
{
	set (double (frame) / frames_per_second);
}

Time::Time (int64_t ticks)
{
	h = ticks / (60 * 60 * 250);
	ticks -= int64_t (h) * 60 * 60 * 250;
	m = ticks / (60 * 250);
	ticks -= int64_t (m) * 60 * 250;
	s = ticks / 250;
	ticks -= int64_t (s) * 250;
	t = ticks;
}

void
Time::set (double ss)
{
	t = (int (round (ss * 1000)) % 1000) / 4;
	s = floor (ss);

	if (s > 60) {
		m = s / 60;
		s -= m * 60;
	}

	if (m > 60) {
		h = m / 60;
		m -= h * 60;
	}
}

Time::Time (string time)
{
	vector<string> b;
	split (b, time, is_any_of (":"));
	if (b.size() != 4) {
		boost::throw_exception (DCPReadError ("unrecognised time specification"));
	}
	
	h = raw_convert<int> (b[0]);
	m = raw_convert<int> (b[1]);
	s = raw_convert<int> (b[2]);
	t = raw_convert<int> (b[3]);
}

bool
dcp::operator== (Time const & a, Time const & b)
{
	return (a.h == b.h && a.m == b.m && a.s == b.s && a.t == b.t);
}

bool
dcp::operator!= (Time const & a, Time const & b)
{
	return !(a == b);
}

bool
dcp::operator<= (Time const & a, Time const & b)
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

bool
dcp::operator< (Time const & a, Time const & b)
{
	if (a.h != b.h) {
		return a.h < b.h;
	}

	if (a.m != b.m) {
		return a.m < b.m;
	}

	if (a.s != b.s) {
		return a.s < b.s;
	}

	if (a.t != b.t) {
		return a.t < b.t;
	}

	return true;
}

bool
dcp::operator> (Time const & a, Time const & b)
{
	if (a.h != b.h) {
		return a.h > b.h;
	}

	if (a.m != b.m) {
		return a.m > b.m;
	}

	if (a.s != b.s) {
		return a.s > b.s;
	}

	if (a.t != b.t) {
		return a.t > b.t;
	}

	return true;
}

ostream &
dcp::operator<< (ostream& s, Time const & t)
{
	s << t.h << ":" << t.m << ":" << t.s << "." << t.t;
	return s;
}

dcp::Time
dcp::operator+ (Time a, Time const & b)
{
	Time r;

	r.t = a.t + b.t;
	if (r.t >= 250) {
		r.t -= 250;
		r.s++;
	}

	r.s += a.s + b.s;
	if (r.s >= 60) {
		r.s -= 60;
		r.m++;
	}

	r.m += a.m + b.m;
	if (r.m >= 60) {
		r.m -= 60;
		r.h++;
	}

	r.h += a.h + b.h;

	return r;
}

dcp::Time
dcp::operator- (Time a, Time const & b)
{
	Time r;

	r.t = a.t - b.t;
	if (r.t < 0) {
		r.t += 250;
		r.s--;
	}

	r.s += (a.s - b.s);
	if (r.s < 0) {
		r.s += 60;
		r.m--;
	}

	r.m += (a.m - b.m);
	if (r.m < 0) {
		r.m += 60;
		r.h--;
	}

	r.h += (a.h - b.h);

	return r;
}

float
dcp::operator/ (Time a, Time const & b)
{
	int64_t const at = a.h * 3600 * 250 + a.m * 60 * 250 + a.s * 250 + a.t;
	int64_t const bt = b.h * 3600 * 250 + b.m * 60 * 250 + b.s * 250 + b.t;
	return float (at) / bt;
}

/** @return A string of the form h:m:s:t */
string
Time::to_string () const
{
	stringstream str;
	str << h << ":" << m << ":" << s << ":" << t;
	return str.str ();
}

/** @return This time in ticks */
int64_t
Time::to_ticks () const
{
	return int64_t(t) + int64_t(s) * 250 + int64_t(m) * 60 * 250 + int64_t(h) * 60 * 60 * 250;
}

double
Time::to_seconds () const
{
	return double (to_ticks ()) / 250;
}
