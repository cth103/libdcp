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

#include "image.h"
#include <boost/test/unit_test.hpp>

using std::string;
using boost::shared_ptr;

class TestImage : public dcp::Image
{
public:
	TestImage (dcp::Size s)
		: Image (s)
	{

	}

	TestImage (TestImage const & other)
		: Image (other)
	{

	}

	TestImage (shared_ptr<const TestImage> other)
		: Image (other)
	{

	}

	uint8_t * const * data () const {
		return 0;
	}

	int const * stride () const {
		return 0;
	}
};

/** Token test for Image class to keep gcov happy */
BOOST_AUTO_TEST_CASE (image_test)
{
	TestImage im (dcp::Size (412, 930));
	BOOST_CHECK_EQUAL (im.size (), dcp::Size (412, 930));

	TestImage im2 (im);
	BOOST_CHECK_EQUAL (im2.size (), im.size ());

	shared_ptr<TestImage> im3 (new TestImage (dcp::Size (1203, 1294)));
	TestImage im4 (im3);
	BOOST_CHECK_EQUAL (im4.size (), im3->size ());
}
