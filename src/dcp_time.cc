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

Time::Time (int frame, int frames_per_second, int tcr_)
{
	set (double (frame) / frames_per_second, tcr_);
}

Time::Time (double seconds)
{
	set (seconds, 24);
}

void
Time::set (double seconds, int tcr_)
{
	s = floor (seconds);
	tcr = tcr_;
	
	e = int (round ((seconds - s) * tcr));

	if (s >= 60) {
		m = s / 60;
		s -= m * 60;
	} else {
		m = 0;
	}

	if (m >= 60) {
		h = m / 60;
		m -= h * 60;
	} else {
		h = 0;
	}
}

Time::Time (string time, int tcr_)
	: tcr (tcr_)
{
	vector<string> b;
	split (b, time, is_any_of (":"));
	if (b.size() != 4) {
		boost::throw_exception (DCPReadError ("unrecognised time specification"));
	}
	
	h = raw_convert<int> (b[0]);
	m = raw_convert<int> (b[1]);
	s = raw_convert<int> (b[2]);
	e = raw_convert<int> (b[3]);
}

bool
dcp::operator== (Time const & a, Time const & b)
{
	return (a.h == b.h && a.m == b.m && a.s == b.s && (a.e * b.tcr) == (b.e * a.tcr));
}

bool
dcp::operator!= (Time const & a, Time const & b)
{
	return !(a == b);
}

bool
dcp::operator<= (Time const & a, Time const & b)
{
	return a < b || a == b;
}

bool
dcp::operator>= (Time const & a, Time const & b)
{
	return a > b || a == b;
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

	if ((a.e * b.tcr) != (b.e * a.tcr)) {
		return (a.e * b.tcr) < (b.e * a.tcr);
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

	if ((a.e * b.tcr) != (b.e * a.tcr)) {
		return (a.e * b.tcr) > (b.e * a.tcr);
	}

	return true;
}

ostream &
dcp::operator<< (ostream& s, Time const & t)
{
	s << t.h << ":" << t.m << ":" << t.s << "." << t.e;
	return s;
}

dcp::Time
dcp::operator+ (Time a, Time b)
{
	Time r;

	/* Make sure we have a common tcr */
	if (a.tcr != b.tcr) {
		a.e *= b.tcr;
		b.e *= a.tcr;
		r.tcr = a.tcr * b.tcr;
	} else {
		r.tcr = a.tcr;
	}

	r.e = a.e + b.e;
	if (r.e >= r.tcr) {
		r.e -= r.tcr;
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
dcp::operator- (Time a, Time b)
{
	Time r;

	/* Make sure we have a common tcr */
	if (a.tcr != b.tcr) {
		a.e *= b.tcr;
		b.e *= a.tcr;
		r.tcr = a.tcr * b.tcr;
	} else {
		r.tcr = a.tcr;
	}
	
	r.e = a.e - b.e;
	if (r.e < 0) {
		r.e += r.tcr;
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
	int64_t const at = a.h * 3600 + a.m * 60 + a.s * float (a.e) / a.tcr;
	int64_t const bt = b.h * 3600 + b.m * 60 + b.s * float (b.e) / b.tcr;
	return float (at) / bt;
}

/** @return A string of the form h:m:s:e padded as in 00:00:00:000 */
string
Time::as_string () const
{
	stringstream str;
	str << setw(2) << setfill('0') << h << ":"
	    << setw(2) << setfill('0') << m << ":"
	    << setw(2) << setfill('0') << s << ":"
	    << setw(3) << setfill('0') << e;
	return str.str ();
}

int64_t
Time::as_editable_units (int tcr_) const
{
	return (int64_t(e) * float (tcr_ / tcr)) + int64_t(s) * tcr_ + int64_t(m) * 60 * tcr_ + int64_t(h) * 60 * 60 * tcr_;
}

double
Time::as_seconds () const
{
	return h * 3600 + m * 60 + s + double(e) / tcr;
}

Time
Time::rebase (int tcr_) const
{
	return Time (h, m, s, rint (float (e) * tcr_ / tcr), tcr_);
}
