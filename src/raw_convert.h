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

#ifndef LIBDCP_RAW_CONVERT_H
#define LIBDCP_RAW_CONVERT_H

#include <sstream>
#include <iomanip>

namespace dcp {

/** A sort-of version of boost::lexical_cast that does uses the "C"
 *  locale (i.e. no thousands separators and a . for the decimal separator).
 */
template <typename P, typename Q>
P
raw_convert (Q v, int precision = 16, bool fixed = false)
{
	std::stringstream s;
	s.imbue (std::locale::classic ());
	s << std::setprecision (precision);
	if (fixed) {
		s << std::fixed;
	}
	s << v;
	P r;
	s >> r;
	return r;
}

};

#endif
