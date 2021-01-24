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


#ifndef LIBDCP_LOCALE_CONVERT_H
#define LIBDCP_LOCALE_CONVERT_H


#include "util.h"
#include <boost/filesystem.hpp>
#include <boost/static_assert.hpp>
#include <string>
#include <cstdio>


namespace dcp {


template <typename P, typename Q>
P
locale_convert (Q, int precision = 16, bool fixed = false)
{
	/* We can't write a generic version of locale_convert; all required
	   versions must be specialised.
	*/
	BOOST_STATIC_ASSERT (sizeof (Q) == 0);
	LIBDCP_UNUSED(precision);
	LIBDCP_UNUSED(fixed);
}

template <>
std::string
locale_convert (unsigned char x, int, bool);

template <>
std::string
locale_convert (unsigned short int x, int, bool);

template <>
std::string
locale_convert (int x, int, bool);

template <>
std::string
locale_convert (unsigned int x, int, bool);

template <>
std::string
locale_convert (long int x, int, bool);

template <>
std::string
locale_convert (unsigned long int x, int, bool);

template <>
std::string
locale_convert (long long int x, int, bool);

template <>
std::string
locale_convert (unsigned long long int x, int, bool);

template <>
std::string
locale_convert (float x, int precision, bool fixed);

template <>
std::string
locale_convert (double x, int precision, bool fixed);

template <>
std::string
locale_convert (std::string x, int, bool);

template <>
std::string
locale_convert (char* x, int, bool);

template <>
std::string
locale_convert (char const * x, int, bool);

template <>
std::string
locale_convert (wchar_t const * x, int, bool);

template <>
std::string
locale_convert (char x, int, bool);

template <>
std::string
locale_convert (boost::filesystem::path x, int, bool);

template <>
unsigned char
locale_convert (std::string x, int, bool);

template <>
unsigned short int
locale_convert (std::string x, int, bool);

template <>
unsigned int
locale_convert (std::string x, int, bool);

template <>
int
locale_convert (std::string x, int, bool);

template <>
long
locale_convert (std::string x, int, bool);

template <>
unsigned long
locale_convert (std::string x, int, bool);

template <>
long long
locale_convert (std::string x, int, bool);

template <>
unsigned long long
locale_convert (std::string x, int, bool);

template <>
float
locale_convert (std::string x, int, bool);

template <>
double
locale_convert (std::string x, int, bool);


}


#endif
