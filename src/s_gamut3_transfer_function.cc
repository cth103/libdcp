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


/** @file  src/s_gamut3_transfer_function.cc
 *  @brief SGamut3TransferFunction class
 */


#include "s_gamut3_transfer_function.h"
#include <cmath>


using std::pow;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using namespace dcp;


double *
SGamut3TransferFunction::make_lut (int bit_depth, bool inverse) const
{
	int const bit_length = int(std::pow(2.0f, bit_depth));
	double* lut = new double[bit_length];
	if (inverse) {
		for (int i = 0; i < bit_length; ++i) {
			auto const p = static_cast<double>(i) / (bit_length - 1);
			if (p >= (0.01125 / 1023)) {
				lut[i] = (420 + log10((p + 0.01) / (0.18 + 0.01)) * 261.5) / 1023;
			} else {
				lut[i] = (p * (171.2102946929 - 95) / 0.01125000 + 95) / 1023;
			}
		}
	} else {
		for (int i = 0; i < bit_length; ++i) {
			auto const p = static_cast<double>(i) / (bit_length - 1);
			if (p >= (171.2102946929 / 1023)) {
				lut[i] = pow(10, ((p * 1023 - 420) / 261.5)) * (0.18 + 0.01) - 0.01;
			} else {
				lut[i] = (p * 1023 - 95) * 0.01125000 / (171.2102946929 - 95);
			}
		}
	}
	return lut;
}


bool
SGamut3TransferFunction::about_equal (shared_ptr<const TransferFunction> other, double) const
{
	auto o = dynamic_pointer_cast<const SGamut3TransferFunction> (other);
	return static_cast<bool> (o);
}
