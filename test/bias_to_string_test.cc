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

/** Test XMLMetadata::bias_to_string */
BOOST_AUTO_TEST_CASE (bias_to_string_test)
{
	BOOST_CHECK_EQUAL (libdcp::XMLMetadata::bias_to_string (30), "+0030");
	BOOST_CHECK_EQUAL (libdcp::XMLMetadata::bias_to_string (60), "+0100");
	BOOST_CHECK_EQUAL (libdcp::XMLMetadata::bias_to_string (61), "+0101");
	BOOST_CHECK_EQUAL (libdcp::XMLMetadata::bias_to_string (7 * 60), "+0700");
	BOOST_CHECK_EQUAL (libdcp::XMLMetadata::bias_to_string (-11 * 60), "-1100");
}
