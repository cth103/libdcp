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

#include "transfer_function.h"

namespace dcp {

/** A transfer function which for an input x gives an output y where
 *
 *  y = x / B                      for x <= threshold
 *  y = ((x + A) / (1 + A))^power  for x >  threshold
 */
class ModifiedGammaTransferFunction : public TransferFunction
{
public:
	ModifiedGammaTransferFunction (float power, float threshold, float A, float B);

	float power () const {
		return _power;
	}

	float threshold () const {
		return _threshold;
	}

	float A () const {
		return _A;
	}

	float B () const {
		return _B;
	}

	bool about_equal (boost::shared_ptr<const TransferFunction>, float epsilon) const;
	
protected:
	float * make_lut (int bit_depth) const;
	
private:
	float _power;
	float _threshold;
	float _A;
	float _B;
};

}
