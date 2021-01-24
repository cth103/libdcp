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


#include "raw_convert.h"
#include "locale_convert.h"
#include <boost/algorithm/string.hpp>


using std::string;
using std::wstring;


/** @param v Numeric value as an ASCII string */
static
string
make_raw (string v)
{
	struct lconv* lc = localeconv ();
	/* thousands_sep may be . so remove them before changing decimal points */
	boost::algorithm::replace_all (v, lc->thousands_sep, "");
	boost::algorithm::replace_all (v, lc->decimal_point, ".");
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
dcp::raw_convert (unsigned char v, int precision, bool fixed)
{
	return make_raw (locale_convert<string> (v, precision, fixed));
}


template <>
string
dcp::raw_convert (unsigned short int v, int precision, bool fixed)
{
	return make_raw (locale_convert<string> (v, precision, fixed));
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
dcp::raw_convert (long v, int precision, bool fixed)
{
	return make_raw (locale_convert<string> (v, precision, fixed));
}


template <>
string
dcp::raw_convert (unsigned long v, int precision, bool fixed)
{
	return make_raw (locale_convert<string> (v, precision, fixed));
}


template <>
string
dcp::raw_convert (long long v, int precision, bool fixed)
{
	return make_raw (locale_convert<string> (v, precision, fixed));
}


template <>
string
dcp::raw_convert (unsigned long long v, int precision, bool fixed)
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
string
dcp::raw_convert (char v, int, bool)
{
	string s;
	s += v;
	return s;
}


template <>
string
dcp::raw_convert (wchar_t const * v, int, bool)
{
	wstring w (v);
	return string (w.begin(), w.end());
}


template <>
unsigned char
dcp::raw_convert (std::string v, int precision, bool fixed)
{
	return locale_convert<unsigned char> (make_local (v), precision, fixed);
}


template <>
unsigned short int
dcp::raw_convert (std::string v, int precision, bool fixed)
{
	return locale_convert<unsigned short int> (make_local (v), precision, fixed);
}


template <>
int
dcp::raw_convert (string v, int precision, bool fixed)
{
	return locale_convert<int> (make_local (v), precision, fixed);
}


template <>
long
dcp::raw_convert (string v, int precision, bool fixed)
{
	return locale_convert<long> (make_local (v), precision, fixed);
}


template <>
unsigned long
dcp::raw_convert (string v, int precision, bool fixed)
{
	return locale_convert<unsigned long> (make_local (v), precision, fixed);
}


template <>
long long
dcp::raw_convert (string v, int precision, bool fixed)
{
	return locale_convert<long long> (make_local (v), precision, fixed);
}


template <>
unsigned long long
dcp::raw_convert (string v, int precision, bool fixed)
{
	return locale_convert<unsigned long long> (make_local (v), precision, fixed);
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
