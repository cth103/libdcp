/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#include "raw_convert.h"
#include "locale_convert.h"
#include <boost/algorithm/string.hpp>

using std::string;

static
string
make_raw (string v)
{
	struct lconv* lc = localeconv ();
	boost::algorithm::replace_all (v, lc->decimal_point, ".");
	boost::algorithm::replace_all (v, lc->thousands_sep, "");
	return v;
}

static
string
make_local (string v)
{
	struct lconv* lc = localeconv ();
	boost::algorithm::replace_all (v, ".", lc->decimal_point);
	/* We hope it's ok not to add in thousands separators here */
	return v;
}

template <>
string
dcp::raw_convert (int v, int precision, bool fixed)
{
	return make_raw (locale_convert<string> (v, precision, fixed));
}

template <>
string
dcp::raw_convert (unsigned int v, int precision, bool fixed)
{
	return make_raw (locale_convert<string> (v, precision, fixed));
}

template <>
string
dcp::raw_convert (int64_t v, int precision, bool fixed)
{
	return make_raw (locale_convert<string> (v, precision, fixed));
}

template <>
string
dcp::raw_convert (uint64_t v, int precision, bool fixed)
{
	return make_raw (locale_convert<string> (v, precision, fixed));
}

template <>
string
dcp::raw_convert (float v, int precision, bool fixed)
{
	return make_raw (locale_convert<string> (v, precision, fixed));
}

template <>
string
dcp::raw_convert (double v, int precision, bool fixed)
{
	return make_raw (locale_convert<string> (v, precision, fixed));
}

template <>
string
dcp::raw_convert (char const * v, int, bool)
{
	return v;
}

template <>
string
dcp::raw_convert (char* v, int, bool)
{
	return v;
}

template <>
string
dcp::raw_convert (string v, int, bool)
{
	return v;
}

template <>
int
dcp::raw_convert (string v, int precision, bool fixed)
{
	return locale_convert<int> (make_local (v), precision, fixed);
}

template <>
int
dcp::raw_convert (char const * v, int precision, bool fixed)
{
	return locale_convert<int> (make_local (v), precision, fixed);
}

template <>
float
dcp::raw_convert (string v, int precision, bool fixed)
{
	return locale_convert<float> (make_local (v), precision, fixed);
}

template <>
float
dcp::raw_convert (char const * v, int precision, bool fixed)
{
	return locale_convert<float> (make_local (v), precision, fixed);
}

template <>
double
dcp::raw_convert (string v, int precision, bool fixed)
{
	return locale_convert<double> (make_local (v), precision, fixed);
}

template <>
double
dcp::raw_convert (char const * v, int precision, bool fixed)
{
	return locale_convert<double> (make_local (v), precision, fixed);
}
