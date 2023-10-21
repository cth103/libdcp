/*
    Copyright (C) 2013-2021 Carl Hetherington <cth@carlh.net>

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


#ifndef LIBDCP_PIECEWISE_LUT_H
#define LIBDCP_PIECEWISE_LUT_H


#include "transfer_function.h"
#include <memory>
#include <vector>


namespace dcp {


class PiecewiseLUT2
{
public:
	PiecewiseLUT2(std::shared_ptr<const TransferFunction> fn, double boundary, int low_bits, int high_bits, bool inverse, int scale)
		: _boundary(boundary)
		, _low(fn->int_lut(0, boundary, low_bits, inverse, scale))
		, _high(fn->int_lut(boundary, 1, high_bits, inverse, scale))
		, _low_scale(static_cast<int>(std::pow(2.0f, low_bits)) - 1)
		, _high_scale(static_cast<int>(std::pow(2.0f, high_bits)) - 1)
	{

	}

	inline int lookup(double x) const {
		return x < _boundary ? _low[lrint((x / _boundary) * _low_scale)] : _high[lrint(((x - _boundary) / (1 - _boundary)) * _high_scale)];
	}

private:
	double _boundary;
	std::vector<int> _low;
	std::vector<int> _high;
	int _low_scale;
	int _high_scale;
};


}

#endif

