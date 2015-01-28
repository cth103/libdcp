/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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
#include "colour_conversion.h"
#include "modified_gamma_transfer_function.h"
#include <boost/test/unit_test.hpp>
#include <cmath>

using std::pow;
using boost::shared_ptr;
using namespace dcp;

static void
check_gamma (shared_ptr<const TransferFunction> tf, int bit_depth, float gamma)
{
	double const * lut = tf->lut (bit_depth);
	int const count = rint (pow (2.0, bit_depth));

	for (int i = 0; i < count; ++i) {
		BOOST_CHECK_CLOSE (lut[i], pow (float(i) / (count - 1), gamma), 0.001);
	}
}

static void
check_modified_gamma (shared_ptr<const TransferFunction> tf, int bit_depth, double power, double threshold, double A, double B)
{
	double const * lut = tf->lut (bit_depth);
	int const count = rint (pow (2.0, bit_depth));

	for (int i = 0; i < count; ++i) {
		double const x = double(i) / (count - 1);
		if (x > threshold) {
			BOOST_CHECK_CLOSE (lut[i], pow ((x + A) / (1 + A), power), 0.001);
		} else {
			BOOST_CHECK_CLOSE (lut[i], (x / B), 0.001);
		}
	}
}

BOOST_AUTO_TEST_CASE (colour_conversion_test1)
{
	ColourConversion cc = ColourConversion::srgb_to_xyz ();

	check_modified_gamma (cc.in(), 8, 2.4, 0.04045, 0.055, 12.92);
	check_modified_gamma (cc.in(), 12, 2.4, 0.04045, 0.055, 12.92);
	check_modified_gamma (cc.in(), 16, 2.4, 0.04045, 0.055, 12.92);

	check_gamma (cc.out(), 8, 1 / 2.6);
	check_gamma (cc.out(), 12, 1 / 2.6);
	check_gamma (cc.out(), 16, 1 / 2.6);
}

BOOST_AUTO_TEST_CASE (colour_conversion_test2)
{
	ColourConversion cc = ColourConversion::rec709_to_xyz ();

	check_modified_gamma (cc.in(), 8, 2.4, 0.081, 0.099, 4.5);
	check_modified_gamma (cc.in(), 12, 2.4, 0.081, 0.099, 4.5);
	check_modified_gamma (cc.in(), 16, 2.4, 0.081, 0.099, 4.5);

	check_gamma (cc.out(), 8, 1 / 2.6);
	check_gamma (cc.out(), 12, 1 / 2.6);
	check_gamma (cc.out(), 16, 1 / 2.6);
}

