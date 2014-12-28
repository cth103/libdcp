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

#include "argb_frame.h"
#include <boost/test/unit_test.hpp>

/** Very simple tests of ARGBFrame */
BOOST_AUTO_TEST_CASE (argb_frame_test)
{
	dcp::ARGBFrame f (dcp::Size (100, 200));

	BOOST_CHECK (f.data() != 0);
	BOOST_CHECK_EQUAL (f.stride(), 100 * 4);
	BOOST_CHECK_EQUAL (f.size(), dcp::Size (100, 200));
}
