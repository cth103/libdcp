/*
    Copyright (C) 2022 Carl Hetherington <cth@carlh.net>

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


#include "file.h"
#include "filesystem.h"
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE (fix_long_path_test)
{
#ifdef LIBDCP_WINDOWS
	BOOST_CHECK_EQUAL(dcp::filesystem::fix_long_path("c:\\foo"), "\\\\?\\c:\\foo");
	BOOST_CHECK_EQUAL(dcp::filesystem::fix_long_path("c:\\foo\\bar"), "\\\\?\\c:\\foo\\bar");
	BOOST_CHECK_EQUAL(dcp::filesystem::fix_long_path("\\\\?\\c:\\foo"), "\\\\?\\c:\\foo");
#else
	BOOST_CHECK_EQUAL(dcp::filesystem::fix_long_path("foo/bar/baz"), "foo/bar/baz");
#endif
}


BOOST_AUTO_TEST_CASE(unfix_long_path_test)
{
#ifdef LIBDCP_WINDOWS
	BOOST_CHECK_EQUAL(dcp::filesystem::unfix_long_path("c:\\foo"), "c:\\foo");
	BOOST_CHECK_EQUAL(dcp::filesystem::unfix_long_path("\\\\?\\c:\\foo"), "c:\\foo");
#else
	BOOST_CHECK_EQUAL(dcp::filesystem::unfix_long_path("c:\\foo"), "c:\\foo");
	BOOST_CHECK_EQUAL(dcp::filesystem::unfix_long_path("\\\\?\\c:\\foo"), "\\\\?\\c:\\foo");
#endif
}


#ifdef LIBDCP_WINDOWS
BOOST_AUTO_TEST_CASE (windows_long_filename_test)
{
	using namespace boost::filesystem;

	/* Make sure current_path() is not already fixed by using our dcp::filesystem version */
	path too_long = dcp::filesystem::current_path() / "build\\test\\a\\really\\very\\long\\filesystem\\path\\indeed\\that\\will\\be\\so\\long\\that\\windows\\cannot\\normally\\cope\\with\\it\\unless\\we\\add\\this\\crazy\\prefix\\and\\then\\magically\\it\\can\\do\\it\\fine\\I\\dont\\really\\know\\why\\its\\like\\that\\but\\hey\\it\\is\\so\\here\\we\\are\\what\\can\\we\\do\\other\\than\\bodge\\it";

	BOOST_CHECK (too_long.string().length() > 260);
	boost::system::error_code ec;
	create_directories (too_long, ec);
	BOOST_CHECK (ec);

	path fixed_path = dcp::filesystem::fix_long_path(too_long);
	create_directories (fixed_path, ec);
	BOOST_CHECK (!ec);

	{
		dcp::File file(too_long / "hello", "w");
		BOOST_REQUIRE (file);
		fprintf (file.get(), "Hello_world");
	}

	{
		dcp::File file(too_long / "hello", "r");
		BOOST_REQUIRE (file);
		char buffer[64];
		fscanf (file.get(), "%63s", buffer);
		BOOST_CHECK_EQUAL (strcmp(buffer, "Hello_world"), 0);
	}
}
#endif


BOOST_AUTO_TEST_CASE(weakly_canonical_test)
{
#ifdef LIBDCP_WINDOWS
	BOOST_CHECK(dcp::filesystem::weakly_canonical("c:\\a\\b\\c") == boost::filesystem::path("c:\\a\\b\\c"));
	BOOST_CHECK(dcp::filesystem::weakly_canonical("c:\\a\\b\\..\\c") == boost::filesystem::path("c:\\a\\c"));
	BOOST_CHECK(dcp::filesystem::weakly_canonical("c:\\a\\b\\..\\c\\.\\d") == boost::filesystem::path("c:\\a\\c\\d"));
	BOOST_CHECK(dcp::filesystem::weakly_canonical("c:\\a\\..\\b\\..\\c") == boost::filesystem::path("c:\\c"));
#else
	BOOST_CHECK(dcp::filesystem::weakly_canonical("/a/b/c") == boost::filesystem::path("/a/b/c"));
	BOOST_CHECK(dcp::filesystem::weakly_canonical("/a/b/../c") == boost::filesystem::path("/a/c"));
	BOOST_CHECK(dcp::filesystem::weakly_canonical("/a/b/../c/./d") == boost::filesystem::path("/a/c/d"));
	BOOST_CHECK(dcp::filesystem::weakly_canonical("/a/../b/../c") == boost::filesystem::path("/c"));
#endif
}
