/*
    Copyright (C) 2016-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/identity_transfer_function.cc
 *  @brief IdentityTransferFunction class
 */


#include "identity_transfer_function.h"
#include <cmath>


using std::pow;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using namespace dcp;


double *
IdentityTransferFunction::make_lut (int bit_depth, bool) const
{
	int const bit_length = int(std::pow(2.0f, bit_depth));
	auto lut = new double[bit_length];
	for (int i = 0; i < bit_length; ++i) {
		lut[i] = double(i) / (bit_length - 1);
	}

	return lut;
}


bool
IdentityTransferFunction::about_equal (shared_ptr<const TransferFunction> other, double) const
{
	auto o = dynamic_pointer_cast<const IdentityTransferFunction>(other);
	return static_cast<bool>(o);
}
