/*
    Copyright (C) 2014-2019 Carl Hetherington <cth@carlh.net>

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

#include "gamma_transfer_function.h"
#include "colour_conversion.h"
#include "modified_gamma_transfer_function.h"
#include <boost/test/unit_test.hpp>
#include <cmath>

using std::pow;
using std::shared_ptr;
using namespace dcp;

static void
check_gamma (shared_ptr<const TransferFunction> tf, int bit_depth, bool inverse, float gamma)
{
	double const * lut = tf->lut (bit_depth, inverse);
	int const count = rint (pow (2.0, bit_depth));

	for (int i = 0; i < count; ++i) {
		BOOST_CHECK_CLOSE (lut[i], pow (float(i) / (count - 1), gamma), 0.001);
	}
}

static void
check_modified_gamma (shared_ptr<const TransferFunction> tf, int bit_depth, bool inverse, double power, double threshold, double A, double B)
{
	double const * lut = tf->lut (bit_depth, inverse);
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

/** Check that the gamma correction LUTs are right for sRGB */
BOOST_AUTO_TEST_CASE (colour_conversion_test1)
{
	ColourConversion cc = ColourConversion::srgb_to_xyz ();

	check_modified_gamma (cc.in(), 8, false, 2.4, 0.04045, 0.055, 12.92);
	check_modified_gamma (cc.in(), 12, false, 2.4, 0.04045, 0.055, 12.92);
	check_modified_gamma (cc.in(), 16, false, 2.4, 0.04045, 0.055, 12.92);

	check_gamma (cc.out(), 8, true, 1 / 2.6);
	check_gamma (cc.out(), 12, true, 1 / 2.6);
	check_gamma (cc.out(), 16, true, 1 / 2.6);
}

/** Check that the gamma correction LUTs are right for REC709 */
BOOST_AUTO_TEST_CASE (colour_conversion_test2)
{
	ColourConversion cc = ColourConversion::rec709_to_xyz ();

	check_gamma (cc.in(), 8, false, 2.2);
	check_gamma (cc.in(), 12, false, 2.2);
	check_gamma (cc.in(), 16, false, 2.2);

	check_gamma (cc.out(), 8, true, 1 / 2.6);
	check_gamma (cc.out(), 12, true, 1 / 2.6);
	check_gamma (cc.out(), 16, true, 1 / 2.6);
}

/** Check that the xyz_to_rgb matrix is the inverse of the rgb_to_xyz one */
BOOST_AUTO_TEST_CASE (colour_conversion_matrix_test)
{
	ColourConversion c = ColourConversion::srgb_to_xyz ();

	boost::numeric::ublas::matrix<double> A = c.rgb_to_xyz ();
	boost::numeric::ublas::matrix<double> B = c.xyz_to_rgb ();

	BOOST_CHECK_CLOSE (A(0, 0) * B(0, 0) + A(0, 1) * B(1, 0) + A(0, 2) * B(2, 0), 1, 0.1);
	BOOST_CHECK (fabs (A(0, 0) * B(0, 1) + A(0, 1) * B(1, 1) + A(0, 2) * B(2, 1)) < 1e-6);
	BOOST_CHECK (fabs (A(0, 0) * B(0, 2) + A(0, 1) * B(1, 2) + A(0, 2) * B(2, 2)) < 1e-6);

	BOOST_CHECK (fabs (A(1, 0) * B(0, 0) + A(1, 1) * B(1, 0) + A(1, 2) * B(2, 0)) < 1e-6);
	BOOST_CHECK_CLOSE (A(1, 0) * B(0, 1) + A(1, 1) * B(1, 1) + A(1, 2) * B(2, 1), 1, 0.1);
	BOOST_CHECK (fabs (A(1, 0) * B(0, 2) + A(1, 1) * B(1, 2) + A(1, 2) * B(2, 2)) < 1e-6);

	BOOST_CHECK (fabs (A(2, 0) * B(0, 0) + A(2, 1) * B(1, 0) + A(2, 2) * B(2, 0)) < 1e-6);
	BOOST_CHECK (fabs (A(2, 0) * B(0, 1) + A(2, 1) * B(1, 1) + A(2, 2) * B(2, 1)) < 1e-6);
	BOOST_CHECK_CLOSE (A(2, 0) * B(0, 2) + A(2, 1) * B(1, 2) + A(2, 2) * B(2, 2), 1, 0.1);
}

BOOST_AUTO_TEST_CASE (colour_conversion_bradford_test)
{
	ColourConversion c = ColourConversion::srgb_to_xyz ();

	/* CIE "A" illuminant from http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html
	   un-normalised using a factor k where k = 1 / (1 + x + z)
	*/
	c.set_adjusted_white (Chromaticity (0.447576324, 0.407443172));

	boost::numeric::ublas::matrix<double> b = c.bradford ();

	/* Check the conversion matrix against the one quoted on brucelindbloom.com */
	BOOST_CHECK_CLOSE (b(0, 0), 1.2164557, 0.1);
	BOOST_CHECK_CLOSE (b(0, 1), 0.1109905, 0.1);
	BOOST_CHECK_CLOSE (b(0, 2), -0.1549325, 0.1);
	BOOST_CHECK_CLOSE (b(1, 0), 0.1533326, 0.1);
	BOOST_CHECK_CLOSE (b(1, 1), 0.9152313, 0.1);
	BOOST_CHECK_CLOSE (b(1, 2), -0.0559953, 0.1);
	BOOST_CHECK_CLOSE (b(2, 0), -0.0239469, 0.1);
	BOOST_CHECK_CLOSE (b(2, 1), 0.0358984, 0.1);
	BOOST_CHECK_CLOSE (b(2, 2), 0.3147529, 0.1);

	/* Same for CIE "B" illuminant */
	c.set_adjusted_white (Chromaticity (0.99072 * 0.351747305, 0.351747305));

	b = c.bradford ();

	BOOST_CHECK_CLOSE (b(0, 0), 1.0641402, 0.1);
	BOOST_CHECK_CLOSE (b(0, 1), 0.0325780, 0.1);
	BOOST_CHECK_CLOSE (b(0, 2), -0.0489436, 0.1);
	BOOST_CHECK_CLOSE (b(1, 0), 0.0446103, 0.1);
	BOOST_CHECK_CLOSE (b(1, 1), 0.9766379, 0.1);
	BOOST_CHECK_CLOSE (b(1, 2), -0.0174854, 0.1);
	BOOST_CHECK_CLOSE (b(2, 0), -0.0078485, 0.1);
	BOOST_CHECK_CLOSE (b(2, 1), 0.0119945, 0.1);
	BOOST_CHECK_CLOSE (b(2, 2), 0.7785377, 0.1);
}
