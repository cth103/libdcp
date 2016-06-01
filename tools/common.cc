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

#include "common.h"
#include "dcp.h"

using std::list;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;

void
dcp::filter_errors (dcp::DCP::ReadErrors& errors, bool ignore_missing_assets)
{
	for (DCP::ReadErrors::iterator i = errors.begin(); i != errors.end(); ) {

		DCP::ReadErrors::iterator tmp = i;
		++tmp;

		if (ignore_missing_assets && dynamic_pointer_cast<MissingAssetError> (*i)) {
			errors.erase (i);
		}

		i = tmp;
	}
}

