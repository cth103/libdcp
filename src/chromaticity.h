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

/** @file  src/chromaticity.h
 *  @brief Chromaticity class.
 */

#ifndef DCP_CHROMATICITY_H
#define DCP_CHROMATICITY_H

#include <cmath>

namespace dcp {

/** @class Chromaticity
 *  @brief A representation of a x,y,z chromaticity, where z = 1 - x - y
 */
class Chromaticity
{
public:
	Chromaticity ()
		: x (0)
		, y (0)
	{}

	Chromaticity (double x_, double y_)
		: x (x_)
		, y (y_)
	{}

	double x;
	double y;

	double z () const {
		return 1 - x - y;
	}

	/** @return true if this Chromaticity's x and y are within epsilon of other */
	bool about_equal (Chromaticity const & other, float epsilon) const {
		return std::fabs (x - other.x) < epsilon && std::fabs (y - other.y) < epsilon;
	}
};

}

#endif
