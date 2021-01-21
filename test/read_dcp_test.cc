/*
    Copyright (C) 2013-2021 Carl Hetherington <cth@carlh.net>

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

#include <boost/test/unit_test.hpp>
#include <boost/optional/optional_io.hpp>
#include "dcp.h"
#include "cpl.h"
#include "stream_operators.h"

using std::list;
using std::shared_ptr;

/** Read a SMPTE DCP that is in git and make sure that basic stuff is read in correctly */
BOOST_AUTO_TEST_CASE (read_dcp_test1)
{
	dcp::DCP d ("test/ref/DCP/dcp_test1");
	d.read ();

	auto cpls = d.cpls ();
	BOOST_CHECK_EQUAL (cpls.size(), 1);

	BOOST_REQUIRE (cpls[0]->annotation_text());
	BOOST_CHECK_EQUAL (cpls[0]->annotation_text().get(), "A Test DCP");
	BOOST_CHECK_EQUAL (cpls[0]->content_kind(), dcp::ContentKind::TRAILER);
	BOOST_REQUIRE (d.standard());
	BOOST_CHECK_EQUAL (d.standard(), dcp::Standard::SMPTE);
}

/** Read an Interop DCP that is in git and make sure that basic stuff is read in correctly */
BOOST_AUTO_TEST_CASE (read_dcp_test2)
{
	dcp::DCP d ("test/ref/DCP/dcp_test3");
	d.read ();

	auto cpls = d.cpls ();
	BOOST_CHECK_EQUAL (cpls.size(), 1);

	BOOST_REQUIRE (cpls[0]->annotation_text());
	BOOST_CHECK_EQUAL (cpls[0]->annotation_text().get(), "Test_FTR-1_F-119_10_2K_20160524_IOP_OV");
	BOOST_CHECK_EQUAL (cpls[0]->content_kind(), dcp::ContentKind::FEATURE);
	BOOST_REQUIRE (d.standard());
	BOOST_CHECK_EQUAL (d.standard(), dcp::Standard::INTEROP);
}
