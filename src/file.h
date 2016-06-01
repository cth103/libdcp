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

/** @file  src/file.h
 *  @brief File class.
 */

#ifndef LIBDCP_FILE_H
#define LIBDCP_FILE_H

#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>

namespace dcp {

/** @class File
 *  @brief Helper class which loads a file into memory.
 */
class File : public boost::noncopyable
{
public:
	File (boost::filesystem::path file);
	~File ();

	uint8_t* data () const {
		return _data;
	}

	int64_t size () const {
		return _size;
	}

private:
	uint8_t* _data; ///< file's data
	int64_t _size;  ///< data size in bytes
};

}

#endif
