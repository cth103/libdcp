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


/** @file  src/raw_convert.h
 *  @brief Methods for conversion to/from string
 */


#ifndef LIBDCP_RAW_CONVERT_H
#define LIBDCP_RAW_CONVERT_H


#include "util.h"
#include <boost/static_assert.hpp>
#include <iomanip>


namespace dcp {


/** A sort-of version of boost::lexical_cast that does uses the "C"
 *  locale (i.e. no thousands separators and a . for the decimal separator).
 */
template <typename P, typename Q>
P
raw_convert (Q, int precision = 16, bool fixed = false)
{
	/* We can't write a generic version of raw_convert; all required
	   versions must be specialised.
	*/
	BOOST_STATIC_ASSERT (sizeof (Q) == 0);
	LIBDCP_UNUSED(precision);
	LIBDCP_UNUSED(fixed);
}

template <>
std::string
raw_convert (unsigned char v, int, bool);

template <>
std::string
raw_convert (unsigned short int v, int, bool);

template <>
std::string
raw_convert (int v, int, bool);

template <>
std::string
raw_convert (unsigned int v, int, bool);

template <>
std::string
raw_convert (long v, int, bool);

template <>
std::string
raw_convert (unsigned long v, int, bool);

template <>
std::string
raw_convert (long long v, int, bool);

template <>
std::string
raw_convert (unsigned long long v, int, bool);

template <>
std::string
raw_convert (float v, int, bool);

template <>
std::string
raw_convert (double v, int, bool);

template <>
std::string
raw_convert (char const * v, int, bool);

template <>
std::string
raw_convert (char* v, int, bool);

template <>
std::string
raw_convert (std::string v, int, bool);

template <>
std::string
raw_convert (wchar_t const * v, int, bool);

template <>
std::string
raw_convert (char v, int, bool);

template <>
unsigned char
raw_convert (std::string v, int, bool);

template <>
unsigned short int
raw_convert (std::string v, int, bool);

template <>
int
raw_convert (std::string v, int, bool);

template <>
long
raw_convert (std::string v, int, bool);

template <>
unsigned long
raw_convert (std::string v, int, bool);

template <>
long long
raw_convert (std::string v, int, bool);

template <>
unsigned long long
raw_convert (std::string v, int, bool);

template <>
int
raw_convert (char const * v, int, bool);

template <>
float
raw_convert (std::string v, int, bool);

template <>
float
raw_convert (char const * v, int, bool);

template <>
double
raw_convert (std::string v, int, bool);

template <>
double
raw_convert (char const * v, int, bool);


}


#endif
