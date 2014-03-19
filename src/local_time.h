/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_LOCAL_TIME_H
#define LIBDCP_LOCAL_TIME_H

#include <string>

class local_time_test;

namespace dcp {

/** I tried to use boost for this, really I did */
class LocalTime
{
public:
	LocalTime ();
	LocalTime (std::string);

	std::string as_string () const;

private:
	friend class ::local_time_test;
	
	int _year;
	int _month;
	int _day;
	int _hour;
	int _minute;
	int _second;
	int _tz_hour;
	int _tz_minute;
};

}

#endif
