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

#include "types.h"
#include "exceptions.h"
#include <boost/test/unit_test.hpp>

/** Test dcp::Fraction */
BOOST_AUTO_TEST_CASE (fraction_test)
{
	dcp::Fraction f (42, 26);
	dcp::Fraction g (42, 26);
	dcp::Fraction h (43, 26);
	dcp::Fraction i (42, 27);

	BOOST_CHECK (f == g);
	BOOST_CHECK (g != h);
	BOOST_CHECK (g != i);

	BOOST_CHECK_THROW (dcp::Fraction ("1 2 3"), dcp::XMLError);
}

