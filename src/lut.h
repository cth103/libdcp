/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_LUT_H
#define LIBDCP_LUT_H

#include <cmath>
#include <boost/utility.hpp>

namespace libdcp {

class LUT : boost::noncopyable
{
public:
	LUT(int bit_depth, float gamma)
		: _lut(0)
		, _bit_depth (bit_depth)
		, _gamma (gamma)
	{
		_lut = new float[int(std::pow(2.0f, _bit_depth))];
	}

	virtual ~LUT() {
		delete[] _lut;
	}
	
	float const * lut() const {
		return _lut;
	}

	int bit_depth () const {
		return _bit_depth;
	}

	float gamma () const {
		return _gamma;
	}

protected:
	float* _lut;
	int _bit_depth;
	float _gamma;
};

}

#endif
