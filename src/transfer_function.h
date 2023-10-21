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


/** @file  src/transfer_function.h
 *  @brief TransferFunction class
 */


#ifndef LIBDCP_TRANSFER_FUNCTION_H
#define LIBDCP_TRANSFER_FUNCTION_H


#include <boost/thread/mutex.hpp>
#include <unordered_map>
#include <memory>
#include <vector>


namespace dcp {


/** @class TransferFunction
 *  @brief A transfer function represented by a lookup table.
 */
class TransferFunction
{
public:
	TransferFunction () {}

	virtual ~TransferFunction () {}

	/** @return A look-up table (of size 2^bit_depth) */
	std::vector<double> const& double_lut(double from, double to, int bit_depth, bool inverse) const;
	std::vector<int> const& int_lut(double from, double to, int bit_depth, bool inverse, int scale) const;

	virtual bool about_equal (std::shared_ptr<const TransferFunction> other, double epsilon) const = 0;

protected:
	virtual std::vector<double> make_double_lut(double from, double to, int bit_depth, bool inverse) const = 0;

private:
	std::vector<int> make_int_lut(double from, double to, int bit_depth, bool inverse, int scale) const;
	std::vector<double> const& double_lut_unlocked(double from, double to, int bit_depth, bool inverse) const;

	struct LUTDescriptor {
		double from;
		double to;
		int bit_depth;
		bool inverse;
		int scale;

		bool operator==(LUTDescriptor const& other) const;
	};

	struct LUTDescriptorHasher {
		std::size_t operator()(LUTDescriptor const& desc) const;
	};

	mutable std::unordered_map<LUTDescriptor, std::vector<double>, LUTDescriptorHasher> _double_luts;
	mutable std::unordered_map<LUTDescriptor, std::vector<int>, LUTDescriptorHasher> _int_luts;
	/** mutex to protect _double_luts and _int_luts */
	mutable boost::mutex _mutex;
};


}


#endif
