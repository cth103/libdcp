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


/** @file  src/fsk.h
 *  @brief FSK class
 */


#include <stdint.h>
#include <vector>


namespace dcp {


/** @class FSK
 *  @brief Create frequency-shift-keyed samples for encoding synchronization signals.
 *
 *  An array of data is given to a FSK object using set_data(), and on calling get()
 *  this data will be returned in a the D-Cinema FSK "format", sample by sample, starting
 *  with the MSB of the first byte in the data array.
 *
 */
class FSK
{
public:
	FSK ();

	void set_data (std::vector<bool> data);

	/** @return the next sample as a 24-bit signed integer */
	int32_t get ();

private:
	std::vector<bool> _data;
	/** current offset into _data */
	int _data_position = 0;
	/** current sample number of the current bit (0-3) */
	int _sample_position = 0;
	/** polarity of the last bit to be written (false for -ve, true for +ve) */
	bool _last_polarity = false;
	/** value of the last bit to be written */
	bool _last_bit = false;
};


}


