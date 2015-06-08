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

#include "exceptions.h"
#include <boost/test/unit_test.hpp>

using std::string;

/** Test exception classes */
BOOST_AUTO_TEST_CASE (exception_test)
{
	BOOST_CHECK_EQUAL (string (dcp::FileError ("foo", "bar", 42).what()), "foo (bar) (error 42)");
	BOOST_CHECK_EQUAL (string (dcp::UnresolvedRefError ("foo").what()), "Unresolved reference to asset id foo");
	BOOST_CHECK_EQUAL (string (dcp::NotEncryptedError ("foo").what()), "foo is not encrypted");
	BOOST_CHECK_EQUAL (string (dcp::ProgrammingError ("foo", 42).what()), "Programming error at foo:42");
	BOOST_CHECK_EQUAL (string (dcp::MissingAssetError ("foo", dcp::MissingAssetError::MAIN_PICTURE).what()), "Missing asset foo for main picture");
	BOOST_CHECK_EQUAL (string (dcp::MissingAssetError ("foo", dcp::MissingAssetError::MAIN_SOUND).what()), "Missing asset foo for main sound");
	BOOST_CHECK_EQUAL (string (dcp::MissingAssetError ("foo", dcp::MissingAssetError::MAIN_SUBTITLE).what()), "Missing asset foo for main subtitle");
}
