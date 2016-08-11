/*
    Copyright (C) 2016 Carl Hetherington <cth@carlh.net>

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

#include "raw_convert.h"
#include "locale_convert.h"
#include <boost/test/unit_test.hpp>
#include <iostream>

using std::string;

BOOST_AUTO_TEST_CASE (raw_convert_test)
{
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (1), "1");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (12), "12");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (12.3), "12.3");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (12.34), "12.34");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (.34), "0.34");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (-.34), "-0.34");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (0.157123125), "0.157123125");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (0.157123125, 4), "0.1571");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (1e50), "1e+50");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (1e-50), "1e-50");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (1.5e12), "1500000000000");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (0.0086), "0.0086");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (0.00086), "0.00086");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (0.000086), "8.6e-05");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> (0.000086, 10, true), "0.0000860000");

	BOOST_CHECK_EQUAL (dcp::raw_convert<int> ("42"), 42);
	BOOST_CHECK_EQUAL (dcp::raw_convert<int> ("42.3"), 42);
	BOOST_CHECK_EQUAL (dcp::raw_convert<int> ("42.7"), 42);

	BOOST_CHECK_EQUAL (dcp::raw_convert<double> ("42"), 42);
	BOOST_CHECK_EQUAL (dcp::raw_convert<double> ("42.3"), 42.3);
	BOOST_CHECK_EQUAL (dcp::raw_convert<double> ("42.7"), 42.7);
	BOOST_CHECK_EQUAL (dcp::raw_convert<double> ("4e8"), 4e8);
	BOOST_CHECK_EQUAL (dcp::raw_convert<double> ("9.1e9"), 9.1e9);
	BOOST_CHECK_EQUAL (dcp::raw_convert<double> ("0.005"), 0.005);

	BOOST_CHECK_CLOSE (dcp::raw_convert<float> ("42"), 42, 0.001);
	BOOST_CHECK_CLOSE (dcp::raw_convert<float> ("42.3"), 42.3, 0.001);
	BOOST_CHECK_CLOSE (dcp::raw_convert<float> ("42.7"), 42.7, 0.001);
	BOOST_CHECK_CLOSE (dcp::raw_convert<float> ("4e8"), 4e8, 0.001);
	BOOST_CHECK_CLOSE (dcp::raw_convert<float> ("9.1e9"), 9.1e9, 0.001);
	BOOST_CHECK_CLOSE (dcp::raw_convert<float> ("0.005"), 0.005, 0.001);

	BOOST_CHECK_EQUAL (dcp::raw_convert<string> ("foo"), "foo");
	BOOST_CHECK_EQUAL (dcp::raw_convert<string> ("foo bar"), "foo bar");
}
