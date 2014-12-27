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
#include <cmath>

using std::pow;
using std::map;
using namespace dcp;

TransferFunction::~TransferFunction ()
{
	for (map<int, float*>::const_iterator i = _luts.begin(); i != _luts.end(); ++i) {
		delete[] i->second;
	}

	_luts.clear ();
}

float const *
TransferFunction::lut (int bit_depth) const
{
	map<int, float*>::const_iterator i = _luts.find (bit_depth);
	if (i != _luts.end ()) {
		return i->second;
	}

	_luts[bit_depth] = make_lut (bit_depth);
	return _luts[bit_depth];
}
