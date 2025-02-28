/*
    Copyright (C) 2025 Carl Hetherington <cth@carlh.net>

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


#include "search.h"
#include "util.h"
#include <boost/test/unit_test.hpp>


using std::vector;


/* boost::filesystem::permissions does not work on Windows as we need. Let's hope the test on
 * Linux/macOS finds problems.
 */

#ifndef LIBDCP_WINDOWS

BOOST_AUTO_TEST_CASE(find_potential_dcps_test)
{
	auto dir = boost::filesystem::path("build/test/find_potential_dcps_test");

	boost::filesystem::remove_all(dir);
	boost::filesystem::create_directories(dir);

	boost::filesystem::create_directories(dir / "foo");
	dcp::write_string_to_file("foo", dir / "foo" / "ASSETMAP");

	boost::filesystem::create_directories(dir / "bar" / "baz");
	dcp::write_string_to_file("foo", dir / "bar" / "baz" / "ASSETMAP.xml");

	boost::filesystem::create_directories(dir / "fred" / "jim" / "sheila" / "brian");
	dcp::write_string_to_file("foo", dir / "fred" / "jim" / "sheila" / "brian" / "ASSETMAP.xml");

	boost::filesystem::create_directories(dir / "fred" / "jim" / "sophie");
	dcp::write_string_to_file("foo", dir / "fred" / "jim" / "sophie" / "ASSETMAP.xml");

	boost::filesystem::permissions(dir / "fred" / "jim" / "sheila", boost::filesystem::remove_perms | boost::filesystem::owner_read | boost::filesystem::owner_exe);

	auto dcp = dcp::find_potential_dcps(dir);

	boost::filesystem::permissions(dir / "fred" / "jim" / "sheila", boost::filesystem::add_perms | boost::filesystem::owner_read | boost::filesystem::owner_exe);

	BOOST_REQUIRE_EQUAL(dcp.size(), 3U);
}

#endif

