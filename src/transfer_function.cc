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
using std::pair;
using std::make_pair;
using boost::shared_ptr;
using namespace dcp;

TransferFunction::~TransferFunction ()
{
	boost::mutex::scoped_lock lm (_mutex);

	for (map<pair<int, bool>, double*>::const_iterator i = _luts.begin(); i != _luts.end(); ++i) {
		delete[] i->second;
	}

	_luts.clear ();
}

double const *
TransferFunction::lut (int bit_depth, bool inverse) const
{
	boost::mutex::scoped_lock lm (_mutex);
	
	map<pair<int, bool>, double*>::const_iterator i = _luts.find (make_pair (bit_depth, inverse));
	if (i != _luts.end ()) {
		return i->second;
	}

	_luts[make_pair(bit_depth, inverse)] = make_lut (bit_depth, inverse);
	return _luts[make_pair(bit_depth, inverse)];
}
