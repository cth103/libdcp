/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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
*/

#include "types.h"
#include "exceptions.h"
#include <boost/test/unit_test.hpp>

/** Very simple tests of Effect-related stuff */
BOOST_AUTO_TEST_CASE (effect_test)
{
	BOOST_CHECK_EQUAL (dcp::effect_to_string (dcp::NONE), "none");
	BOOST_CHECK_EQUAL (dcp::effect_to_string (dcp::BORDER), "border");
	BOOST_CHECK_EQUAL (dcp::effect_to_string (dcp::SHADOW), "shadow");

	BOOST_CHECK_THROW (dcp::effect_to_string ((dcp::Effect) 42), dcp::MiscError);

	BOOST_CHECK_EQUAL (dcp::string_to_effect ("none"), dcp::NONE);
	BOOST_CHECK_EQUAL (dcp::string_to_effect ("border"), dcp::BORDER);
	BOOST_CHECK_EQUAL (dcp::string_to_effect ("shadow"), dcp::SHADOW);
	BOOST_CHECK_THROW (dcp::string_to_effect ("fish"), dcp::DCPReadError);
}
