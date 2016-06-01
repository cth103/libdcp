/*
    Copyright (C) 2015 Carl Hetherington <cth@carlh.net>

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

#include "data.h"
#include "util.h"
#include "exceptions.h"
#include <cstdio>
#include <cerrno>

using boost::shared_array;
using namespace dcp;

Data::Data ()
	: _size (0)
{

}

Data::Data (int size)
	: _data (new uint8_t[size])
	, _size (size)
{

}

Data::Data (uint8_t const * data, int size)
	: _data (new uint8_t[size])
	, _size (size)
{
	memcpy (_data.get(), data, size);
}

Data::Data (shared_array<uint8_t> data, int size)
	: _data (data)
	, _size (size)
{

}

Data::Data (boost::filesystem::path file)
{
	_size = boost::filesystem::file_size (file);
	_data.reset (new uint8_t[_size]);

	FILE* f = fopen_boost (file, "rb");
	if (!f) {
		throw FileError ("could not open file for reading", file, errno);
	}

	size_t const r = fread (_data.get(), 1, _size, f);
	if (r != size_t (_size)) {
		fclose (f);
		throw FileError ("could not read from file", file, errno);
	}

	fclose (f);
}

void
Data::write (boost::filesystem::path file) const
{
	FILE* f = fopen_boost (file, "wb");
	if (!f) {
		throw FileError ("could not write to file", file, errno);
	}
	size_t const r = fwrite (_data.get(), 1, _size, f);
	if (r != size_t (_size)) {
		fclose (f);
		throw FileError ("could not write to file", file, errno);
	}
	fclose (f);
}

void
Data::write_via_temp (boost::filesystem::path temp, boost::filesystem::path final) const
{
	write (temp);
	boost::filesystem::rename (temp, final);
}
