/*
    Copyright (C) 2014-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/colour_conversion.cc
 *  @brief ColourConversion class
 */


#include "colour_conversion.h"
#include "gamma_transfer_function.h"
#include "modified_gamma_transfer_function.h"
#include "s_gamut3_transfer_function.h"
#include "identity_transfer_function.h"
#include "dcp_assert.h"
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/io.hpp>


using std::shared_ptr;
using std::make_shared;
using boost::optional;
using namespace dcp;


ColourConversion const &
ColourConversion::srgb_to_xyz ()
{
	static auto c = new ColourConversion (
		make_shared<ModifiedGammaTransferFunction>(2.4, 0.04045, 0.055, 12.92),
		YUVToRGB::REC601,
		Chromaticity (0.64, 0.33),
		Chromaticity (0.3, 0.6),
		Chromaticity (0.15, 0.06),
		Chromaticity::D65 (),
		optional<Chromaticity> (),
		make_shared<GammaTransferFunction>(2.6)
		);
	return *c;
}

ColourConversion const &
ColourConversion::rec601_to_xyz ()
{
	static auto c = new ColourConversion (
		make_shared<GammaTransferFunction>(2.2),
		YUVToRGB::REC601,
		Chromaticity (0.64, 0.33),
		Chromaticity (0.3, 0.6),
		Chromaticity (0.15, 0.06),
		Chromaticity::D65 (),
		optional<Chromaticity> (),
		make_shared<GammaTransferFunction>(2.6)
		);
	return *c;
}

ColourConversion const &
ColourConversion::rec709_to_xyz ()
{
	static auto c = new ColourConversion (
		make_shared<GammaTransferFunction>(2.2),
		YUVToRGB::REC709,
		Chromaticity (0.64, 0.33),
		Chromaticity (0.3, 0.6),
		Chromaticity (0.15, 0.06),
		Chromaticity::D65 (),
		optional<Chromaticity> (),
		make_shared<GammaTransferFunction>(2.6)
		);
	return *c;
}

ColourConversion const &
ColourConversion::p3_to_xyz ()
{
	static auto c = new ColourConversion (
		make_shared<GammaTransferFunction>(2.6),
		YUVToRGB::REC709,
		Chromaticity (0.68, 0.32),
		Chromaticity (0.265, 0.69),
		Chromaticity (0.15, 0.06),
		Chromaticity (0.314, 0.351),
		optional<Chromaticity> (),
		make_shared<GammaTransferFunction>(2.6)
		);
	return *c;
}

ColourConversion const &
ColourConversion::rec1886_to_xyz ()
{
	/* According to Olivier on DCP-o-matic bug #832, Rec. 1886 is Rec. 709 with
	   2.4 gamma, so here goes ...
	*/
	static auto c = new ColourConversion (
		make_shared<GammaTransferFunction>(2.4),
		YUVToRGB::REC709,
		Chromaticity (0.64, 0.33),
		Chromaticity (0.3, 0.6),
		Chromaticity (0.15, 0.06),
		Chromaticity::D65 (),
		optional<Chromaticity> (),
		make_shared<GammaTransferFunction>(2.6)
		);
	return *c;
}

ColourConversion const &
ColourConversion::rec2020_to_xyz ()
{
	static auto c = new ColourConversion (
		make_shared<GammaTransferFunction>(2.4),
		YUVToRGB::REC709,
		Chromaticity (0.708, 0.292),
		Chromaticity (0.170, 0.797),
		Chromaticity (0.131, 0.046),
		Chromaticity::D65 (),
		optional<Chromaticity> (),
		make_shared<GammaTransferFunction>(2.6)
		);
	return *c;
}

/** Sony S-Gamut3/S-Log3 */
ColourConversion const &
ColourConversion::s_gamut3_to_xyz ()
{
	static auto c = new ColourConversion (
		make_shared<SGamut3TransferFunction>(),
		YUVToRGB::REC709,
		Chromaticity (0.73, 0.280),
		Chromaticity (0.140, 0.855),
		Chromaticity (0.100, -0.050),
		Chromaticity::D65 (),
		optional<Chromaticity> (),
		make_shared<IdentityTransferFunction>()
		);
	return *c;
}



ColourConversion::ColourConversion (
	shared_ptr<const TransferFunction> in,
	YUVToRGB yuv_to_rgb,
	Chromaticity red,
	Chromaticity green,
	Chromaticity blue,
	Chromaticity white,
	optional<Chromaticity> adjusted_white,
	shared_ptr<const TransferFunction> out
	)
	: _in (in)
	, _yuv_to_rgb (yuv_to_rgb)
	, _red (red)
	, _green (green)
	, _blue (blue)
	, _white (white)
	, _adjusted_white (adjusted_white)
	, _out (out)
{

}


bool
ColourConversion::about_equal (ColourConversion const & other, float epsilon) const
{
	if (!_in->about_equal (other._in, epsilon) ||
	    _yuv_to_rgb != other._yuv_to_rgb ||
	    !_red.about_equal (other._red, epsilon) ||
	    !_green.about_equal (other._green, epsilon) ||
	    !_blue.about_equal (other._blue, epsilon) ||
	    !_white.about_equal (other._white, epsilon) ||
	    (!_out && other._out) ||
	    (_out && !other._out) ||
	    (_out && !_out->about_equal (other._out, epsilon))) {
		return false;
	}

	if (!_adjusted_white && !other._adjusted_white) {
		return true;
	}

	if (
		_adjusted_white && other._adjusted_white &&
		fabs (_adjusted_white.get().x - other._adjusted_white.get().x) < epsilon &&
		fabs (_adjusted_white.get().y - other._adjusted_white.get().y) < epsilon
		) {
		return true;
	}

	/* Otherwise one has an adjusted white and other hasn't, or they both have but different */
	return false;
}


boost::numeric::ublas::matrix<double>
ColourConversion::rgb_to_xyz () const
{
	/* See doc/design/colour.tex */

	double const D = (_red.x - _white.x) * (_white.y - _blue.y) - (_white.x - _blue.x) * (_red.y - _white.y);
	double const E = (_white.x - _green.x) * (_red.y - _white.y) - (_red.x - _white.x) * (_white.y - _green.y);
	double const F = (_white.x - _green.x) * (_white.y - _blue.y) - (_white.x - _blue.x) * (_white.y - _green.y);
	double const P = _red.y + _green.y * D / F + _blue.y * E / F;

	boost::numeric::ublas::matrix<double> C (3, 3);
	C(0, 0) = _red.x / P;
	C(0, 1) = _green.x * D / (F * P);
	C(0, 2) = _blue.x * E / (F * P);
	C(1, 0) = _red.y / P;
	C(1, 1) = _green.y * D / (F * P);
	C(1, 2) = _blue.y * E / (F * P);
	C(2, 0) = _red.z() / P;
	C(2, 1) = _green.z() * D / (F * P);
	C(2, 2) = _blue.z() * E / (F * P);
	return C;
}


boost::numeric::ublas::matrix<double>
ColourConversion::xyz_to_rgb () const
{
	boost::numeric::ublas::matrix<double> A (rgb_to_xyz());

	/* permutation matrix for the LU-factorization */
	boost::numeric::ublas::permutation_matrix<std::size_t> pm (A.size1());

	/* perform LU-factorization */
	int const r = lu_factorize (A, pm);
	DCP_ASSERT (r == 0);

	/* create identity matrix of inverse */
	boost::numeric::ublas::matrix<double> xyz_to_rgb (3, 3);
	xyz_to_rgb.assign (boost::numeric::ublas::identity_matrix<double> (A.size1()));

	/* backsubstitute to get the inverse */
	lu_substitute (A, pm, xyz_to_rgb);

	return xyz_to_rgb;
}


boost::numeric::ublas::matrix<double>
ColourConversion::bradford () const
{
	if (!_adjusted_white || fabs (_adjusted_white.get().x) < 1e-6 || fabs (_adjusted_white.get().y) < 1e-6) {
		boost::numeric::ublas::matrix<double> B = boost::numeric::ublas::zero_matrix<double> (3, 3);
		B(0, 0) = 1;
		B(1, 1) = 1;
		B(2, 2) = 1;
		return B;
	}

	/* See doc/design/colour.tex */

	boost::numeric::ublas::matrix<double> M (3, 3);
	M(0, 0) = 0.8951;
	M(0, 1) = 0.2664;
	M(0, 2) = -0.1614;
	M(1, 0) = -0.7502;
	M(1, 1) = 1.7135;
	M(1, 2) = 0.0367;
	M(2, 0) = 0.0389;
	M(2, 1) = -0.0685;
	M(2, 2) = 1.0296;

	boost::numeric::ublas::matrix<double> Mi (3, 3);
	Mi(0, 0) = 0.9869929055;
	Mi(0, 1) = -0.1470542564;
	Mi(0, 2) = 0.1599626517;
	Mi(1, 0) = 0.4323052697;
	Mi(1, 1) = 0.5183602715;
	Mi(1, 2) = 0.0492912282;
	Mi(2, 0) = -0.0085286646;
	Mi(2, 1) = 0.0400428217;
	Mi(2, 2) = 0.9684866958;

	boost::numeric::ublas::matrix<double> Gp (3, 1);
	Gp(0, 0) = _white.x / _white.y;
	Gp(1, 0) = 1;
	Gp(2, 0) = (1 - _white.x - _white.y) / _white.y;

	boost::numeric::ublas::matrix<double> G = boost::numeric::ublas::prod (M, Gp);

	boost::numeric::ublas::matrix<double> Hp (3, 1);
	Hp(0, 0) = _adjusted_white.get().x / _adjusted_white.get().y;
	Hp(1, 0) = 1;
	Hp(2, 0) = (1 - _adjusted_white.get().x - _adjusted_white.get().y) / _adjusted_white.get().y;

	boost::numeric::ublas::matrix<double> H = boost::numeric::ublas::prod (M, Hp);

	boost::numeric::ublas::matrix<double> C = boost::numeric::ublas::zero_matrix<double> (3, 3);
	C(0, 0) = H(0, 0) / G(0, 0);
	C(1, 1) = H(1, 0) / G(1, 0);
	C(2, 2) = H(2, 0) / G(2, 0);

	boost::numeric::ublas::matrix<double> CM = boost::numeric::ublas::prod (C, M);
	return boost::numeric::ublas::prod (Mi, CM);
}
