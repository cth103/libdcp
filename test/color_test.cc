/*
    Copyright (C) 2012-2013 Carl Hetherington <cth@carlh.net>

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

#include "util.h"

/* Check that libdcp::Color works */
BOOST_AUTO_TEST_CASE (color)
{
	libdcp::Color c ("FFFF0000");

	BOOST_CHECK_EQUAL (c.r, 255);
	BOOST_CHECK_EQUAL (c.g, 0);
	BOOST_CHECK_EQUAL (c.b, 0);
	BOOST_CHECK_EQUAL (c.to_argb_string(), "FFFF0000");

	c = libdcp::Color ("FF00FF00");

	BOOST_CHECK_EQUAL (c.r, 0);
	BOOST_CHECK_EQUAL (c.g, 255);
	BOOST_CHECK_EQUAL (c.b, 0);
	BOOST_CHECK_EQUAL (c.to_argb_string(), "FF00FF00");

	c = libdcp::Color ("FF0000FF");

	BOOST_CHECK_EQUAL (c.r, 0);
	BOOST_CHECK_EQUAL (c.g, 0);
	BOOST_CHECK_EQUAL (c.b, 255);
	BOOST_CHECK_EQUAL (c.to_argb_string(), "FF0000FF");
}
