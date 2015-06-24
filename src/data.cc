/*
    Copyright (C) 2015 Carl Hetherington <cth@carlh.net>

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

#include "data.h"
#include "util.h"
#include "exceptions.h"
#include <cstdio>

using namespace dcp;

Data::Data (boost::filesystem::path file)
{
	FILE* f = fopen_boost (file, "rb");
	if (!f) {
		throw FileError ("could not open file for reading", file, errno);
	}

	size = boost::filesystem::file_size (file);
	data.reset (new uint8_t[size]);
	size_t const read = fread (data.get(), 1, size, f);
	fclose (f);

	if (read != size) {
		throw FileError ("could not read file", file, -1);
	}
}
