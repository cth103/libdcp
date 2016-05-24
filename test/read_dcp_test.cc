/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

#include <boost/test/unit_test.hpp>
#include <boost/optional/optional_io.hpp>
#include "dcp.h"
#include "cpl.h"

using std::list;
using boost::shared_ptr;

/** Read a SMPTE DCP that is in git and make sure that basic stuff is read in correctly */
BOOST_AUTO_TEST_CASE (read_dcp_test1)
{
	dcp::DCP d ("test/ref/DCP/dcp_test1");
	d.read ();

	list<shared_ptr<dcp::CPL> > cpls = d.cpls ();
	BOOST_CHECK_EQUAL (cpls.size(), 1);

	BOOST_CHECK_EQUAL (cpls.front()->annotation_text(), "A Test DCP");
	BOOST_CHECK_EQUAL (cpls.front()->content_kind(), dcp::FEATURE);
	BOOST_REQUIRE (d.standard());
	BOOST_CHECK_EQUAL (d.standard(), dcp::SMPTE);
}

/** Read an Interop DCP that is in git and make sure that basic stuff is read in correctly */
BOOST_AUTO_TEST_CASE (read_dcp_test2)
{
	dcp::DCP d ("test/ref/DCP/dcp_test3");
	d.read ();

	list<shared_ptr<dcp::CPL> > cpls = d.cpls ();
	BOOST_CHECK_EQUAL (cpls.size(), 1);

	BOOST_CHECK_EQUAL (cpls.front()->annotation_text(), "Test_FTR-1_F-119_10_2K_20160524_IOP_OV");
	BOOST_CHECK_EQUAL (cpls.front()->content_kind(), dcp::FEATURE);
	BOOST_REQUIRE (d.standard());
	BOOST_CHECK_EQUAL (d.standard(), dcp::INTEROP);
}
