/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

/** @file  src/file.cc
 *  @brief File class.
 */

#include "file.h"
#include "util.h"

using namespace dcp;

/** Read a file into memory.
 *  @param file to read.
 */
File::File (boost::filesystem::path file)
{
	_size = boost::filesystem::file_size (file);
	_data = new uint8_t[_size];
	FILE* f = dcp::fopen_boost (file, "r");
	assert (f);
	fread (_data, 1, _size, f);
	fclose (f);
}

/** File destructor */
File::~File ()
{
	delete[] _data;
}
