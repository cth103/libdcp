/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#include "gamma_lut.h"
#include "lut_cache.h"
#include <cmath>

using namespace dcp;

LUTCache<GammaLUT> GammaLUT::cache;

GammaLUT::GammaLUT (int bit_depth, float gamma, bool linearised)
	: _bit_depth (bit_depth)
	, _gamma (gamma)
	, _linearised (linearised)
{
	_lut = new float[int(std::pow(2.0f, _bit_depth))];
	int const bit_length = pow (2, _bit_depth);

	if (_linearised) {
		for (int i = 0; i < bit_length; ++i) {
			float const p = static_cast<float> (i) / (bit_length - 1);
			if (p > 0.04045) {
				_lut[i] = pow ((p + 0.055) / 1.055, gamma);
			} else {
				_lut[i] = p / 12.92;
			}
		}
	} else {
		for (int i = 0; i < bit_length; ++i) {
			_lut[i] = pow(float(i) / (bit_length - 1), gamma);
		}
	}
}
