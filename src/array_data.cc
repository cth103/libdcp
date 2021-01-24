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


/** @file  src/array_data.cc
 *  @brief ArrayData class
 */


#include "array_data.h"
#include "util.h"
#include "exceptions.h"
#include <cstdio>
#include <cerrno>


using boost::shared_array;
using namespace dcp;


ArrayData::ArrayData ()
{

}


ArrayData::ArrayData (int size)
	: _data (new uint8_t[size])
	, _size (size)
{

}


ArrayData::ArrayData (uint8_t const * data, int size)
	: _data (new uint8_t[size])
	, _size (size)
{
	memcpy (_data.get(), data, size);
}


ArrayData::ArrayData (shared_array<uint8_t> data, int size)
	: _data (data)
	, _size (size)
{

}


ArrayData::ArrayData (boost::filesystem::path file)
{
	_size = boost::filesystem::file_size (file);
	_data.reset (new uint8_t[_size]);

	auto f = fopen_boost (file, "rb");
	if (!f) {
		throw FileError ("could not open file for reading", file, errno);
	}

	auto const r = fread (_data.get(), 1, _size, f);
	fclose (f);
	if (r != static_cast<size_t>(_size)) {
		throw FileError ("could not read from file", file, errno);
	}
}
