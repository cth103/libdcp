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

#include "util.h"
#include "exceptions.h"
#include <boost/test/unit_test.hpp>

using std::stringstream;

/** Check that dcp::Colour works */
BOOST_AUTO_TEST_CASE (colour)
{
	dcp::Colour z;

	BOOST_CHECK_EQUAL (z.r, 0);
	BOOST_CHECK_EQUAL (z.g, 0);
	BOOST_CHECK_EQUAL (z.b, 0);
	BOOST_CHECK_EQUAL (z.to_argb_string(), "FF000000");
	
	dcp::Colour c ("FFFF0000");

	BOOST_CHECK_EQUAL (c.r, 255);
	BOOST_CHECK_EQUAL (c.g, 0);
	BOOST_CHECK_EQUAL (c.b, 0);
	BOOST_CHECK_EQUAL (c.to_argb_string(), "FFFF0000");

	dcp::Colour d = dcp::Colour ("FF00FF00");

	BOOST_CHECK_EQUAL (d.r, 0);
	BOOST_CHECK_EQUAL (d.g, 255);
	BOOST_CHECK_EQUAL (d.b, 0);
	BOOST_CHECK_EQUAL (d.to_argb_string(), "FF00FF00");

	dcp::Colour e = dcp::Colour ("FF0000FF");

	BOOST_CHECK_EQUAL (e.r, 0);
	BOOST_CHECK_EQUAL (e.g, 0);
	BOOST_CHECK_EQUAL (e.b, 255);
	BOOST_CHECK_EQUAL (e.to_argb_string(), "FF0000FF");

	BOOST_CHECK (c != d);

	BOOST_CHECK_THROW (dcp::Colour ("001234"), dcp::XMLError);

	stringstream s;
	s << c;
	BOOST_CHECK_EQUAL (s.str(), "(255, 0, 0)");
}
