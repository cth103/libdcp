/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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
*/

/** @file  src/transfer_function.h
 *  @brief TransferFunction class.
 */

#ifndef LIBDCP_TRANSFER_FUNCTION_H
#define LIBDCP_TRANSFER_FUNCTION_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <map>

namespace dcp {

/** @class TransferFunction
 *  @brief A transfer function represented by a lookup table.
 */
class TransferFunction : public boost::noncopyable
{
public:
	virtual ~TransferFunction ();

	/** @return A look-up table (of size 2^bit_depth) whose values range from 0 to 1 */
	double const * lut (int bit_depth, bool inverse) const;

	virtual bool about_equal (boost::shared_ptr<const TransferFunction> other, double epsilon) const = 0;

protected:
	/** Make a LUT and return an array allocated by new */
	virtual double * make_lut (int bit_depth, bool inverse) const = 0;

private:
	mutable std::map<std::pair<int, bool>, double*> _luts;
	/** mutex to protect _luts */
	mutable boost::mutex _mutex;
};

}

#endif
