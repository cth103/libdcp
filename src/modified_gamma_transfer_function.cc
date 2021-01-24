/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/modified_gamma_transfer_function.cc
 *  @brief ModifiedGammaTransferFunction class
 */


#include "modified_gamma_transfer_function.h"
#include <cmath>


using std::pow;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using namespace dcp;


ModifiedGammaTransferFunction::ModifiedGammaTransferFunction (double power, double threshold, double A, double B)
	: _power (power)
	, _threshold (threshold)
	, _A (A)
	, _B (B)
{

}


double *
ModifiedGammaTransferFunction::make_lut (int bit_depth, bool inverse) const
{
	int const bit_length = int(std::pow(2.0f, bit_depth));
	double* lut = new double[bit_length];
	if (inverse) {
		double const threshold = _threshold / _B;
		for (int i = 0; i < bit_length; ++i) {
			double const p = static_cast<double>(i) / (bit_length - 1);
			if (p > threshold) {
				lut[i] = (1 + _A) * pow (p, 1 / _power) - _A;
			} else {
				lut[i] = p * _B;
			}
		}
	} else {
		for (int i = 0; i < bit_length; ++i) {
			double const p = static_cast<double>(i) / (bit_length - 1);
			if (p > _threshold) {
				lut[i] = pow ((p + _A) / (1 + _A), _power);
			} else {
				lut[i] = p / _B;
			}
		}
	}

	return lut;
}


bool
ModifiedGammaTransferFunction::about_equal (shared_ptr<const TransferFunction> other, double epsilon) const
{
	auto o = dynamic_pointer_cast<const ModifiedGammaTransferFunction>(other);
	if (!o) {
		return false;
	}

	return (
		fabs(_power - o->_power) < epsilon &&
	        fabs(_threshold - o->_threshold) < epsilon &&
		fabs(_A - o->_A) < epsilon &&
		fabs(_B - o->_B) < epsilon
		);
}
