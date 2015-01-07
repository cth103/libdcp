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
using boost::shared_ptr;
using namespace dcp;

TransferFunction::TransferFunction (bool inverse)
	: _inverse (inverse)
{

}

TransferFunction::~TransferFunction ()
{
	boost::mutex::scoped_lock lm (_mutex);

	for (map<int, double*>::const_iterator i = _luts.begin(); i != _luts.end(); ++i) {
		delete[] i->second;
	}

	_luts.clear ();
}

double const *
TransferFunction::lut (int bit_depth) const
{
	boost::mutex::scoped_lock lm (_mutex);
	
	map<int, double*>::const_iterator i = _luts.find (bit_depth);
	if (i != _luts.end ()) {
		return i->second;
	}

	_luts[bit_depth] = make_lut (bit_depth);
	return _luts[bit_depth];
}

bool
TransferFunction::about_equal (shared_ptr<const TransferFunction> other, double) const
{
	return _inverse == other->_inverse;
}
