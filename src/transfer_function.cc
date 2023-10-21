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


using std::make_pair;
using std::pair;
using std::pow;
using std::shared_ptr;
using std::vector;
using namespace dcp;


vector<double> const&
TransferFunction::double_lut(double from, double to, int bit_depth, bool inverse) const
{
	boost::mutex::scoped_lock lm (_mutex);
	return double_lut_unlocked(from, to, bit_depth, inverse);
}


vector<double> const&
TransferFunction::double_lut_unlocked(double from, double to, int bit_depth, bool inverse) const
{
	auto const descriptor = LUTDescriptor{from, to, bit_depth, inverse, 1};

	auto i = _double_luts.find(descriptor);
	if (i != _double_luts.end()) {
		return i->second;
	}

	_double_luts[descriptor] = make_double_lut(from, to, bit_depth, inverse);
	return _double_luts[descriptor];
}


vector<int> const&
TransferFunction::int_lut(double from, double to, int bit_depth, bool inverse, int scale) const
{
	boost::mutex::scoped_lock lm (_mutex);

	auto const descriptor = LUTDescriptor{from, to, bit_depth, inverse, scale};

	auto i = _int_luts.find(descriptor);
	if (i != _int_luts.end()) {
		return i->second;
	}

	_int_luts[descriptor] = make_int_lut(from, to, bit_depth, inverse, scale);
	return _int_luts[descriptor];
}


/* Caller must hold lock on _mutex */
vector<int>
TransferFunction::make_int_lut(double from, double to, int bit_depth, bool inverse, int scale) const
{
	auto source_lut = double_lut_unlocked(from, to, bit_depth, inverse);
	auto const size = source_lut.size();
	vector<int> lut(size);
	for (size_t i = 0; i < size; ++i) {
		lut[i] = lrint(source_lut[i] * scale);
	}

	return lut;
}


bool
TransferFunction::LUTDescriptor::operator==(TransferFunction::LUTDescriptor const& other) const
{
	return from == other.from && to == other.to && bit_depth == other.bit_depth && inverse == other.inverse && scale == other.scale;
}


std::size_t
TransferFunction::LUTDescriptorHasher::operator()(TransferFunction::LUTDescriptor const& desc) const
{
	return std::hash<double>()(desc.from) ^
		std::hash<double>()(desc.to) ^
		std::hash<int>()(desc.bit_depth) ^
		std::hash<bool>()(desc.inverse) ^
		std::hash<int>()(desc.scale);
}

