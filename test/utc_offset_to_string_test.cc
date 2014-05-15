/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

#include <boost/test/unit_test.hpp>
#include "metadata.h"
#include "util.h"

/** Test libdcp::utc_offset_to_string */
BOOST_AUTO_TEST_CASE (utc_offset_to_string_test)
{
	BOOST_CHECK_EQUAL (libdcp::utc_offset_to_string (boost::posix_time::time_duration (0, 30, 0, 0)), "+00:30");
	BOOST_CHECK_EQUAL (libdcp::utc_offset_to_string (boost::posix_time::time_duration (0, 60, 0, 0)), "+01:00");
	BOOST_CHECK_EQUAL (libdcp::utc_offset_to_string (boost::posix_time::time_duration (0, 61, 0, 0)), "+01:01");
	BOOST_CHECK_EQUAL (libdcp::utc_offset_to_string (boost::posix_time::time_duration (7, 0, 0, 0)), "+07:00");
	BOOST_CHECK_EQUAL (libdcp::utc_offset_to_string (boost::posix_time::time_duration (-11, 0, 0, 0)), "-11:00");
}
