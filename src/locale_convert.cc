/*
    Copyright (C) 2016-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/locale_convert.cc
 *  @brief Methods to convert to/from string using the current locale
 */


#include "locale_convert.h"
#include <string>
#include <inttypes.h>


using std::string;
using std::wstring;


template<>
string
dcp::locale_convert (unsigned char x, int, bool)
{
	char buffer[64];
	snprintf (buffer, sizeof(buffer), "%hhd", x);
	return buffer;
}


template<>
string
dcp::locale_convert (unsigned short int x, int, bool)
{
	char buffer[64];
	snprintf (buffer, sizeof(buffer), "%hd", x);
	return buffer;
}


template<>
string
dcp::locale_convert (int x, int, bool)
{
	char buffer[64];
	snprintf (buffer, sizeof(buffer), "%d", x);
	return buffer;
}


template<>
string
dcp::locale_convert (unsigned int x, int, bool)
{
	char buffer[64];
	snprintf (buffer, sizeof(buffer), "%u", x);
	return buffer;
}


template<>
string
dcp::locale_convert (long int x, int, bool)
{
	char buffer[64];
#ifdef LIBDCP_WINDOWS
	__mingw_snprintf (buffer, sizeof(buffer), "%ld", x);
#else
	snprintf (buffer, sizeof(buffer), "%ld", x);
#endif
	return buffer;
}


template<>
string
dcp::locale_convert (unsigned long int x, int, bool)
{
	char buffer[64];
	snprintf (buffer, sizeof(buffer), "%lu", x);
	return buffer;
}


template<>
string
dcp::locale_convert (long long int x, int, bool)
{
	char buffer[64];
#ifdef LIBDCP_WINDOWS
	__mingw_snprintf (buffer, sizeof(buffer), "%lld", x);
#else
	snprintf (buffer, sizeof(buffer), "%lld", x);
#endif
	return buffer;
}


template<>
string
dcp::locale_convert (unsigned long long int x, int, bool)
{
	char buffer[64];
#ifdef LIBDCP_WINDOWS
	__mingw_snprintf (buffer, sizeof(buffer), "%llu", x);
#else
	snprintf (buffer, sizeof(buffer), "%llu", x);
#endif
	return buffer;
}


template<>
string
dcp::locale_convert (float x, int precision, bool fixed)
{
	char format[64];
	if (fixed) {
		snprintf (format, sizeof(format), "%%.%df", precision);
	} else {
		snprintf (format, sizeof(format), "%%.%dg", precision);
	}
	char buffer[64];
	snprintf (buffer, sizeof(buffer), format, x);
	return buffer;
}


template<>
string
dcp::locale_convert (double x, int precision, bool fixed)
{
	char format[64];
	if (fixed) {
		snprintf (format, sizeof(format), "%%.%df", precision);
	} else {
		snprintf (format, sizeof(format), "%%.%dg", precision);
	}
	char buffer[64];
	snprintf (buffer, sizeof(buffer), format, x);
	return buffer;
}


template<>
string
dcp::locale_convert (string x, int, bool)
{
	return x;
}


template<>
string
dcp::locale_convert (char* x, int, bool)
{
	return x;
}


template<>
string
dcp::locale_convert (char const * x, int, bool)
{
	return x;
}


template<>
string
dcp::locale_convert (wchar_t const * x, int, bool)
{
	wstring s (x);
	return string (s.begin(), s.end());
}


template<>
string
dcp::locale_convert (char x, int, bool)
{
	string s;
	s += x;
	return s;
}


template<>
string
dcp::locale_convert (boost::filesystem::path x, int, bool)
{
	return x.string();
}


template<>
unsigned char
dcp::locale_convert (string x, int, bool)
{
	unsigned char y = 0;
	sscanf (x.c_str(), "%hhu", &y);
	return y;
}


template<>
unsigned short int
dcp::locale_convert (string x, int, bool)
{
	unsigned short int y = 0;
	sscanf (x.c_str(), "%hu", &y);
	return y;
}


template<>
unsigned int
dcp::locale_convert (string x, int, bool)
{
	unsigned int y = 0;
	sscanf (x.c_str(), "%u", &y);
	return y;
}


template<>
int
dcp::locale_convert (string x, int, bool)
{
	int y = 0;
	sscanf (x.c_str(), "%d", &y);
	return y;
}


template<>
long
dcp::locale_convert (string x, int, bool)
{
	long int y = 0;
	sscanf (x.c_str(), "%ld", &y);
	return y;
}


template<>
unsigned long
dcp::locale_convert (string x, int, bool)
{
	unsigned long y = 0;
#ifdef LIBDCP_WINDOWS
	__mingw_sscanf (x.c_str(), "%lud", &y);
#else
	sscanf (x.c_str(), "%lud", &y);
#endif
	return y;
}


template<>
long long
dcp::locale_convert (string x, int, bool)
{
	long long y = 0;
#ifdef LIBDCP_WINDOWS
	__mingw_sscanf (x.c_str(), "%lld", &y);
#else
	sscanf (x.c_str(), "%lld", &y);
#endif
	return y;
}


template<>
unsigned long long
dcp::locale_convert (string x, int, bool)
{
	unsigned long long y = 0;
#ifdef LIBDCP_WINDOWS
	__mingw_sscanf (x.c_str(), "%llud", &y);
#else
	sscanf (x.c_str(), "%llud", &y);
#endif
	return y;
}


template<>
float
dcp::locale_convert (string x, int, bool)
{
	float y = 0;
	sscanf (x.c_str(), "%f", &y);
	return y;
}


template<>
double
dcp::locale_convert (string x, int, bool)
{
	double y = 0;
	sscanf (x.c_str(), "%lf", &y);
	return y;
}
