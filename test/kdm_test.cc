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
#include "kdm.h"
#include "xml/kdm_smpte.h"

using std::list;
using boost::shared_ptr;

BOOST_AUTO_TEST_CASE (kdm_test)
{
	libdcp::KDM kdm (
		"test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml",
		"test/data/private.key"
		);

	list<libdcp::KDMKey> keys = kdm.keys ();
	
	BOOST_CHECK_EQUAL (keys.size(), 2);

	BOOST_CHECK_EQUAL (keys.front().cpl_id(), "eece17de-77e8-4a55-9347-b6bab5724b9f");
	BOOST_CHECK_EQUAL (keys.front().key_id(), "4ac4f922-8239-4831-b23b-31426d0542c4");
	BOOST_CHECK_EQUAL (keys.front().not_valid_before(), "2013-07-06T20:04:58+00:00");
	BOOST_CHECK_EQUAL (keys.front().not_valid_after(), "2023-07-02T20:04:56+00:00");
	BOOST_CHECK_EQUAL (keys.front().key().hex(), "8a2729c3e5b65c45d78305462104c3fb");

	BOOST_CHECK_EQUAL (keys.back().cpl_id(), "eece17de-77e8-4a55-9347-b6bab5724b9f");
	BOOST_CHECK_EQUAL (keys.back().key_id(), "73baf5de-e195-4542-ab28-8a465f7d4079");
	BOOST_CHECK_EQUAL (keys.back().not_valid_before(), "2013-07-06T20:04:58+00:00");
	BOOST_CHECK_EQUAL (keys.back().not_valid_after(), "2023-07-02T20:04:56+00:00");
	BOOST_CHECK_EQUAL (keys.back().key().hex(), "5327fb7ec2e807bd57059615bf8a169d");
}

/* Check that we can read in a KDM and then write it back out again the same */
BOOST_AUTO_TEST_CASE (kdm_passthrough_test)
{
	libdcp::xml::DCinemaSecurityMessage kdm (
		"test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml"
		);

	shared_ptr<xmlpp::Document> doc = kdm.as_xml ();
	doc->write_to_file_formatted ("build/kdm.xml", "UTF-8");
	int const r = system (
		"xmldiff -c test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml build/kdm.xml"
		);

#ifdef LIBDCP_WINDOWS	
	BOOST_CHECK_EQUAL (r, 0);
#else	
	BOOST_CHECK_EQUAL (WEXITSTATUS (r), 0);
#endif	
}
