/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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
#include <libxml++/libxml++.h>
#include "encrypted_kdm.h"
#include "decrypted_kdm.h"
#include "util.h"

using std::list;
using std::stringstream;
using boost::shared_ptr;

BOOST_AUTO_TEST_CASE (kdm_test)
{
	dcp::DecryptedKDM kdm (
		dcp::EncryptedKDM ("test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml"),
		dcp::file_to_string ("test/data/private.key")
		);

	list<dcp::DecryptedKDMKey> keys = kdm.keys ();
	
	BOOST_CHECK_EQUAL (keys.size(), 2);

	BOOST_CHECK_EQUAL (keys.front().cpl_id(), "eece17de-77e8-4a55-9347-b6bab5724b9f");
	BOOST_CHECK_EQUAL (keys.front().id(), "4ac4f922-8239-4831-b23b-31426d0542c4");
	BOOST_CHECK_EQUAL (keys.front().key().hex(), "8a2729c3e5b65c45d78305462104c3fb");

	BOOST_CHECK_EQUAL (keys.back().cpl_id(), "eece17de-77e8-4a55-9347-b6bab5724b9f");
	BOOST_CHECK_EQUAL (keys.back().id(), "73baf5de-e195-4542-ab28-8a465f7d4079");
	BOOST_CHECK_EQUAL (keys.back().key().hex(), "5327fb7ec2e807bd57059615bf8a169d");
}

/* Check that we can read in a KDM and then write it back out again the same */
BOOST_AUTO_TEST_CASE (kdm_passthrough_test)
{
	dcp::EncryptedKDM kdm (
		"test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml"
		);

	shared_ptr<xmlpp::DomParser> parser (new xmlpp::DomParser ());
	stringstream s;
	s << kdm.as_xml ();
	parser->parse_stream (s);
	parser->get_document()->write_to_file_formatted ("build/kdm.xml", "UTF-8");
	int const r = system (
		"xmldiff -c test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml build/kdm.xml"
		);

#ifdef LIBDCP_WINDOWS	
	BOOST_CHECK_EQUAL (r, 0);
#else	
	BOOST_CHECK_EQUAL (WEXITSTATUS (r), 0);
#endif	
}
