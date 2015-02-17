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

#include "colour_conversion.h"
#include "gamma_transfer_function.h"
#include "modified_gamma_transfer_function.h"
#include "colour_matrix.h"

using boost::shared_ptr;
using namespace dcp;

ColourConversion const &
ColourConversion::srgb_to_xyz ()
{
	static ColourConversion* c = new ColourConversion (
		shared_ptr<const TransferFunction> (new ModifiedGammaTransferFunction (false, 2.4, 0.04045, 0.055, 12.92)),
		dcp::colour_matrix::rgb_to_xyz,
		shared_ptr<const TransferFunction> (new GammaTransferFunction (true, 2.6))
		);
	return *c;
}

ColourConversion const &
ColourConversion::xyz_to_srgb ()
{
	static ColourConversion* c = new ColourConversion (
		shared_ptr<const TransferFunction> (new GammaTransferFunction (false, 2.6)),
		dcp::colour_matrix::xyz_to_rgb,
		shared_ptr<const TransferFunction> (new ModifiedGammaTransferFunction (true, 2.4, 0.04045, 0.055, 12.92))
		);
	return *c;
}

ColourConversion const &
ColourConversion::rec709_to_xyz ()
{
	static ColourConversion* c = new ColourConversion (
		shared_ptr<const TransferFunction> (new ModifiedGammaTransferFunction (false, 2.4, 0.081, 0.099, 4.5)),
		dcp::colour_matrix::rgb_to_xyz,
		shared_ptr<const TransferFunction> (new GammaTransferFunction (true, 2.6))
		);
	return *c;
}

ColourConversion::ColourConversion (
	shared_ptr<const TransferFunction> in,
	double const matrix[3][3],
	shared_ptr<const TransferFunction> out
	)
	: _in (in)
	, _matrix (3, 3)
	, _out (out)
{
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			_matrix (i, j) = matrix[i][j];
		}
	}
}

bool
ColourConversion::about_equal (ColourConversion const & other, float epsilon) const
{
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			if (fabs (_matrix(i, j) - other._matrix(i, j)) > epsilon) {
				return false;
			}
		}
	}

	return _in->about_equal (other._in, epsilon) && _out->about_equal (other._out, epsilon);
}
