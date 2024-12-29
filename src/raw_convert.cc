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
#include <fmt/format.h>
#include <fast_float/fast_float.h>
#include <boost/algorithm/string.hpp>


using std::string;
using std::wstring;


template <>
string
dcp::raw_convert(unsigned char v, int, bool)
{
	return fmt::to_string(v);
}


template <>
string
dcp::raw_convert(unsigned short int v, int, bool)
{
	return fmt::to_string(v);
}


template <>
string
dcp::raw_convert(int v, int, bool)
{
	return fmt::to_string(v);
}


template <>
string
dcp::raw_convert(unsigned int v, int, bool)
{
	return fmt::to_string(v);
}


template <>
string
dcp::raw_convert(long v, int, bool)
{
	return fmt::to_string(v);
}


template <>
string
dcp::raw_convert(unsigned long v, int, bool)
{
	return fmt::to_string(v);
}


template <>
string
dcp::raw_convert(long long v, int, bool)
{
	return fmt::to_string(v);
}


template <>
string
dcp::raw_convert(unsigned long long v, int, bool)
{
	return fmt::to_string(v);
}


static
void
make_format_string(char* buffer, int buffer_length, int precision, bool fixed)
{
	if (fixed) {
		snprintf(buffer, buffer_length, "{:.%df}", precision);
	} else {
		snprintf(buffer, buffer_length, "{:.%d}", precision);
	}
}


template <>
string
dcp::raw_convert (float v, int precision, bool fixed)
{
	if (precision < 16) {
		char format[16];
		make_format_string(format, 16, precision, fixed);
		return fmt::format(format, v);
	}

	return fmt::to_string(v);
}


template <>
string
dcp::raw_convert (double v, int precision, bool fixed)
{
	if (precision < 16) {
		char format[16];
		make_format_string(format, 16, precision, fixed);
		return fmt::format(format, v);
	}

	return fmt::to_string(v);
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


template <typename T>
T
convert_with_fast_float(string v)
{
	T result;
	auto const answer = fast_float::from_chars(v.data(), v.data() + v.size(), result);
	if (answer.ec != std::errc()) {
		return 0;
	}

	return result;
}


template <>
unsigned char
dcp::raw_convert(string v, int, bool)
{
	return convert_with_fast_float<unsigned char>(v);
}


template <>
unsigned short int
dcp::raw_convert(string v, int, bool)
{
	return convert_with_fast_float<unsigned short int>(v);
}


template <>
int
dcp::raw_convert(string v, int, bool)
{
	return convert_with_fast_float<int>(v);
}


template <>
long
dcp::raw_convert(string v, int, bool)
{
	return convert_with_fast_float<long>(v);
}


template <>
unsigned long
dcp::raw_convert(string v, int, bool)
{
	return convert_with_fast_float<unsigned long>(v);
}


template <>
long long
dcp::raw_convert(string v, int, bool)
{
	return convert_with_fast_float<long long>(v);
}


template <>
unsigned long long
dcp::raw_convert(string v, int, bool)
{
	return convert_with_fast_float<unsigned long long>(v);
}


template <>
int
dcp::raw_convert(char* v, int, bool)
{
	return convert_with_fast_float<int>(string(v));
}


template <>
int
dcp::raw_convert(char const * v, int, bool)
{
	return convert_with_fast_float<int>(string(v));
}


template <>
float
dcp::raw_convert(string v, int, bool)
{
	return convert_with_fast_float<float>(v);
}


template <>
float
dcp::raw_convert(char const * v, int, bool)
{
	return convert_with_fast_float<float>(string(v));
}


template <>
double
dcp::raw_convert(string v, int, bool)
{
	return convert_with_fast_float<double>(v);
}


template <>
double
dcp::raw_convert(char const * v, int, bool)
{
	return convert_with_fast_float<double>(string(v));
}
