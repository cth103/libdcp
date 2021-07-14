/*
    Copyright (C) 2016-2021 Carl Hetherington <cth@carlh.net>

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

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/


#include "array_data.h"
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
	dcp::ArrayData data (N);
	auto p = data.data();
	for (int i = 0; i < N; ++i) {
		*p++ = rand() & 0xff;
	}
	data.write ("build/test/random");

	/* Hash it */
	BOOST_CHECK_EQUAL (dcp::make_digest("build/test/random", boost::bind(&progress, _1)), "GKbk/V3fcRtP5MaPdSmAGNbKkaU=");
}
