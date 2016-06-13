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

#ifndef LIBDCP_DATA_H
#define LIBDCP_DATA_H

#include <boost/shared_array.hpp>
#include <boost/filesystem.hpp>
#include <stdint.h>

namespace dcp {

class Data
{
public:
	Data ();
	explicit Data (int size);
	Data (uint8_t const * data, int size);
	Data (boost::shared_array<uint8_t> data, int size);
	explicit Data (boost::filesystem::path file);

	virtual ~Data () {}

	void write (boost::filesystem::path file) const;
	void write_via_temp (boost::filesystem::path temp, boost::filesystem::path final) const;

	boost::shared_array<uint8_t> data () const {
		return _data;
	}

	int size () const {
		return _size;
	}

	void set_size (int s) {
		_size = s;
	}

private:
	boost::shared_array<uint8_t> _data;
	/** amount of `valid' data in _data; the array may be larger */
	int _size;
};

}

#endif
