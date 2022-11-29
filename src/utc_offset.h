/*
    Copyright (C) 2014-2021 Carl Hetherington <cth@carlh.net>

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


#ifndef LIBDCP_UTC_OFFSET_H
#define LIBDCP_UTC_OFFSET_H


namespace dcp {


class UTCOffset
{
public:
	UTCOffset() {}
	UTCOffset(int hour, int minute)
		: _hour(hour)
		, _minute(minute)
	{}

	int hour() const {
		return _hour;
	}

	int minute() const {
		return _minute;
	}

	void set_hour(int hour);
	void set_minute(int hour);

private:
	/** offset such that the equivalent time in UTC can be calculated by
	 *  doing local_time - (_hour, _minute).  For example:
	 *  local time    UTC    _hour    _minute
	 *       09:00  10:00       -1          0
	 *       09:00  10:30       -1        -30
	 *       03:00  02:00        1          0
	 *       03:00  01:30        1         30
	 */
	int _hour = 0;
	int _minute = 0;
};


bool operator==(UTCOffset const& a, UTCOffset const& b);
bool operator!=(UTCOffset const& a, UTCOffset const& b);


}


#endif

