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

*/

/** @file  src/file.cc
 *  @brief File class.
 */

#include "file.h"
#include "util.h"
#include "dcp_assert.h"
#include <stdio.h>

using namespace dcp;

/** Read a file into memory.
 *  @param file to read.
 */
File::File (boost::filesystem::path file)
{
	_size = boost::filesystem::file_size (file);
	_data = new uint8_t[_size];
	FILE* f = dcp::fopen_boost (file, "rb");
	DCP_ASSERT (f);
	fread (_data, 1, _size, f);
	fclose (f);
}

/** File destructor */
File::~File ()
{
	delete[] _data;
}
