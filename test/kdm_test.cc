/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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
*/

#include "encrypted_kdm.h"
#include "decrypted_kdm.h"
#include "util.h"
#include <libxml++/libxml++.h>
#include <boost/test/unit_test.hpp>

using std::list;
using boost::shared_ptr;

/** Check reading and decryption of a KDM */
BOOST_AUTO_TEST_CASE (kdm_test)
{
	dcp::DecryptedKDM kdm (
		dcp::EncryptedKDM (
			dcp::file_to_string ("test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml")
			),
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

/** Check that we can read in a KDM and then write it back out again the same */
BOOST_AUTO_TEST_CASE (kdm_passthrough_test)
{
	dcp::EncryptedKDM kdm (
		dcp::file_to_string ("test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml")
		);

	shared_ptr<xmlpp::DomParser> parser (new xmlpp::DomParser ());
	parser->parse_memory (kdm.as_xml ());
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

/** Test some of the utility methods of DecryptedKDM */
BOOST_AUTO_TEST_CASE (decrypted_kdm_test)
{
	uint8_t* data = new uint8_t[16];
	uint8_t* p = data;
	dcp::DecryptedKDM::put_uuid (&p, "8971c838-d0c3-405d-bc57-43afa9d91242");

	BOOST_CHECK_EQUAL (data[0], 0x89);
	BOOST_CHECK_EQUAL (data[1], 0x71);
	BOOST_CHECK_EQUAL (data[2], 0xc8);
	BOOST_CHECK_EQUAL (data[3], 0x38);
	BOOST_CHECK_EQUAL (data[4], 0xd0);
	BOOST_CHECK_EQUAL (data[5], 0xc3);
	BOOST_CHECK_EQUAL (data[6], 0x40);
	BOOST_CHECK_EQUAL (data[7], 0x5d);
	BOOST_CHECK_EQUAL (data[8], 0xbc);
	BOOST_CHECK_EQUAL (data[9], 0x57);
	BOOST_CHECK_EQUAL (data[10], 0x43);
	BOOST_CHECK_EQUAL (data[11], 0xaf);
	BOOST_CHECK_EQUAL (data[12], 0xa9);
	BOOST_CHECK_EQUAL (data[13], 0xd9);
	BOOST_CHECK_EQUAL (data[14], 0x12);
	BOOST_CHECK_EQUAL (data[15], 0x42);

	p = data;
	BOOST_CHECK_EQUAL (dcp::DecryptedKDM::get_uuid (&p), "8971c838-d0c3-405d-bc57-43afa9d91242");

	delete[] data;
}
