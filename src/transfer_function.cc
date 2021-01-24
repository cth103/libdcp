/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/* @file  src/transfer_function.cc
 * @brief TransferFunction
 */


#include "transfer_function.h"
#include <cmath>


using std::pow;
using std::map;
using std::pair;
using std::make_pair;
using std::shared_ptr;
using namespace dcp;


TransferFunction::~TransferFunction ()
{
	boost::mutex::scoped_lock lm (_mutex);

	for (auto const& i: _luts) {
		delete[] i.second;
	}

	_luts.clear ();
}


double const *
TransferFunction::lut (int bit_depth, bool inverse) const
{
	boost::mutex::scoped_lock lm (_mutex);

	auto i = _luts.find (make_pair (bit_depth, inverse));
	if (i != _luts.end ()) {
		return i->second;
	}

	_luts[make_pair(bit_depth, inverse)] = make_lut (bit_depth, inverse);
	return _luts[make_pair(bit_depth, inverse)];
}
