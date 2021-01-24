/*
    Copyright (C) 2015-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/data.cc
 *  @brief Data class
 */


#include "data.h"
#include "exceptions.h"
#include "util.h"
#include <cstdio>
#include <cerrno>


using namespace dcp;


void
Data::write (boost::filesystem::path file) const
{
	auto f = fopen_boost (file, "wb");
	if (!f) {
		throw FileError ("could not write to file", file, errno);
	}
	size_t const r = fwrite (data(), 1, size(), f);
	fclose (f);
	if (r != size_t(size())) {
		throw FileError ("could not write to file", file, errno);
	}
}


void
Data::write_via_temp (boost::filesystem::path temp, boost::filesystem::path final) const
{
	write (temp);
	boost::filesystem::rename (temp, final);
}


bool
dcp::operator== (Data const & a, Data const & b)
{
	return (a.size() == b.size() && memcmp(a.data(), b.data(), a.size()) == 0);
}


bool
dcp::operator!= (Data const & a, Data const & b)
{
	return (a.size() != b.size() || memcmp(a.data(), b.data(), a.size()) != 0);
}

