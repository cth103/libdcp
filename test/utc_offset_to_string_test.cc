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

#include "metadata.h"

/** Test XMLMetadata::utc_offset_to_string */
BOOST_AUTO_TEST_CASE (utc_offset_to_string_test)
{
	BOOST_CHECK_EQUAL (libdcp::XMLMetadata::utc_offset_to_string (30), "+00:30");
	BOOST_CHECK_EQUAL (libdcp::XMLMetadata::utc_offset_to_string (60), "+01:00");
	BOOST_CHECK_EQUAL (libdcp::XMLMetadata::utc_offset_to_string (61), "+01:01");
	BOOST_CHECK_EQUAL (libdcp::XMLMetadata::utc_offset_to_string (7 * 60), "+07:00");
	BOOST_CHECK_EQUAL (libdcp::XMLMetadata::utc_offset_to_string (-11 * 60), "-11:00");
}
