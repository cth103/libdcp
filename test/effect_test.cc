/*
    Copyright (C) 2014-2019 Carl Hetherington <cth@carlh.net>

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

#include "types.h"
#include "exceptions.h"
#include <boost/test/unit_test.hpp>

/** Very simple tests of Effect-related stuff */
BOOST_AUTO_TEST_CASE (effect_test)
{
	BOOST_CHECK_EQUAL (dcp::effect_to_string(dcp::Effect::NONE), "none");
	BOOST_CHECK_EQUAL (dcp::effect_to_string(dcp::Effect::BORDER), "border");
	BOOST_CHECK_EQUAL (dcp::effect_to_string(dcp::Effect::SHADOW), "shadow");

	BOOST_CHECK_THROW (dcp::effect_to_string((dcp::Effect) 42), dcp::MiscError);

	BOOST_CHECK_EQUAL (dcp::string_to_effect("none"), dcp::Effect::NONE);
	BOOST_CHECK_EQUAL (dcp::string_to_effect("border"), dcp::Effect::BORDER);
	BOOST_CHECK_EQUAL (dcp::string_to_effect("shadow"), dcp::Effect::SHADOW);
	BOOST_CHECK_THROW (dcp::string_to_effect("fish"), dcp::ReadError);
}
