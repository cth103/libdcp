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

/** @file  src/gamma_transfer_function.h
 *  @brief GammaTransferFunction class.
 */

#include "transfer_function.h"

namespace dcp {

/** A description of a gamma function of the f(x) = x^g where
 *  g is the gamma.
 */
class GammaTransferFunction : public TransferFunction
{
public:
	GammaTransferFunction (double gamma);

	double gamma () const {
		return _gamma;
	}

	bool about_equal (boost::shared_ptr<const TransferFunction> other, double epsilon) const;

protected:
	double * make_lut (int bit_depth, bool inverse) const;

private:
	double _gamma;
};

}
