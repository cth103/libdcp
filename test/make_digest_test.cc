/*
    Copyright (C) 2016 Carl Hetherington <cth@carlh.net>

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

#include "data.h"
#include "util.h"
#include <boost/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <sys/time.h>

void progress (float)
{

}

/** Check SHA1 digests */
BOOST_AUTO_TEST_CASE (make_digest_test)
{
	/* Make a big file with some random data */
	srand (1);
	int const N = 256 * 1024 * 1024;
	dcp::Data data (N);
	uint8_t* p = data.data().get();
	for (int i = 0; i < N; ++i) {
		*p++ = rand() & 0xff;
	}
	data.write ("build/test/random");

	/* Hash it */
	BOOST_CHECK_EQUAL (dcp::make_digest ("build/test/random", boost::bind (&progress, _1)), "GKbk/V3fcRtP5MaPdSmAGNbKkaU=");
}
