/*
    Copyright (C) 2020-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/fsk.cc
 *  @brief FSK class
 */


#include "fsk.h"
#include <iostream>


using std::cout;
using std::vector;
using namespace dcp;


FSK::FSK ()
{

}


void
FSK::set_data (vector<bool> data)
{
	_data = data;
	_data_position = _sample_position = 0;
}


int32_t
FSK::get ()
{
	static int const twenty_four_bit = 8388608; // 2^23

	static int const lut[4][2] = {
		// sample 0
		{
			int( 0.03827 * twenty_four_bit), // 0
			int( 0.07071 * twenty_four_bit), // 1
		},
		// sample 1
		{
			int( 0.09239 * twenty_four_bit), // 0
			int( 0.07071 * twenty_four_bit), // 1
		},
		// sample 2
		{
			int( 0.09239 * twenty_four_bit), // 0
			int(-0.07071 * twenty_four_bit), // 1
		},
		// sample 3
		{
			int( 0.03827 * twenty_four_bit), // 0
			int(-0.07071 * twenty_four_bit), // 1
		}
	};

	/* The bit we are working on */
	bool const bit = _data[_data_position];
	/* Get the +ve version of the required sample */
	int sample = lut[_sample_position][bit];

	bool polarity = _last_polarity;
	if (_sample_position == 0 && _last_bit == false) {
		/* We're starting a new bit, and the last one was 0 so we need to flip
		 * the polarity we are using.
		 */
		polarity = !polarity;
	}

	/* Obey the required polarity for this sample */
	if (polarity == false) {
		sample = -sample;
	}

	/* Get ready for next time */

	_last_bit = bit;
	_last_polarity = polarity;

	++_sample_position;
	if (_sample_position == 4) {
		_sample_position = 0;
		++_data_position;
	}

	return sample;
}

