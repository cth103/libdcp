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

#include "gamma_transfer_function.h"
#include <cmath>

using std::pow;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using namespace dcp;

GammaTransferFunction::GammaTransferFunction (double gamma)
	: _gamma (gamma)
{

}

double *
GammaTransferFunction::make_lut (int bit_depth, bool inverse) const
{
	int const bit_length = int(std::pow(2.0f, bit_depth));
	double* lut = new double[bit_length];
	double const gamma = inverse ? (1 / _gamma) : _gamma;
	for (int i = 0; i < bit_length; ++i) {
		lut[i] = pow(double(i) / (bit_length - 1), gamma);
	}

	return lut;
}

bool
GammaTransferFunction::about_equal (shared_ptr<const TransferFunction> other, double epsilon) const
{
	shared_ptr<const GammaTransferFunction> o = dynamic_pointer_cast<const GammaTransferFunction> (other);
	if (!o) {
		return false;
	}

	return fabs (_gamma - o->_gamma) < epsilon;
}
