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
#include "certificate_chain.h"
#include "util.h"
#include "test.h"
#include <libcxml/cxml.h>
#include <libxml++/libxml++.h>
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

using std::list;
using std::string;
using std::vector;
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

/** Check that <KeyType> tags have the scope attribute.
 *  Wolfgang Woehl believes this is compulsory and I am more-or-less inclined to agree.
 */
BOOST_AUTO_TEST_CASE (kdm_key_type_scope)
{
	dcp::EncryptedKDM kdm (
		dcp::file_to_string ("test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml")
		);

	cxml::Document doc;
	doc.read_string (kdm.as_xml ());

	list<cxml::NodePtr> typed_key_ids = doc.node_child("AuthenticatedPublic")->
		node_child("RequiredExtensions")->
		node_child("KDMRequiredExtensions")->
		node_child("KeyIdList")->
		node_children("TypedKeyId");

	BOOST_FOREACH (cxml::NodePtr i, typed_key_ids) {
		BOOST_FOREACH (cxml::NodePtr j, i->node_children("KeyType")) {
			BOOST_CHECK (j->string_attribute("scope") == "http://www.smpte-ra.org/430-1/2006/KDM#kdm-key-type");
		}
	}
}

static cxml::ConstNodePtr
kdm_forensic_test (cxml::Document& doc, int picture, int audio)
{
	dcp::DecryptedKDM decrypted (
		dcp::EncryptedKDM (
			dcp::file_to_string ("test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml")
			),
		dcp::file_to_string ("test/data/private.key")
		);

	shared_ptr<dcp::CertificateChain> signer(new dcp::CertificateChain(dcp::file_to_string("test/data/certificate_chain")));
	signer->set_key(dcp::file_to_string("test/data/private.key"));

	dcp::EncryptedKDM kdm = decrypted.encrypt (
		signer, signer->leaf(), vector<dcp::Certificate>(), dcp::MODIFIED_TRANSITIONAL_1, picture, audio
		);

	/* Check that we can pass this through correctly */
	BOOST_CHECK_EQUAL (kdm.as_xml(), dcp::EncryptedKDM(kdm.as_xml()).as_xml());

	doc.read_string (kdm.as_xml());

	return doc.node_child("AuthenticatedPublic")->
		node_child("RequiredExtensions")->
		node_child("KDMRequiredExtensions")->
		optional_node_child("ForensicMarkFlagList");
}

/** Check ForensicMarkFlagList handling: disable picture and all audio */
BOOST_AUTO_TEST_CASE (kdm_forensic_test1)
{
	cxml::Document doc;
	cxml::ConstNodePtr forensic = kdm_forensic_test(doc, -1, -1);
	BOOST_REQUIRE (forensic);
	list<cxml::NodePtr> flags = forensic->node_children("ForensicMarkFlag");
	BOOST_REQUIRE_EQUAL (flags.size(), 2);
	BOOST_CHECK_EQUAL (flags.front()->content(), "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-picture-disable");
	BOOST_CHECK_EQUAL (flags.back()->content(), "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-audio-disable");
}

/** Check ForensicMarkFlagList handling: disable picture but not audio */
BOOST_AUTO_TEST_CASE (kdm_forensic_test2)
{
	cxml::Document doc;
	cxml::ConstNodePtr forensic = kdm_forensic_test(doc, -1, 0);
	BOOST_REQUIRE (forensic);
	list<cxml::NodePtr> flags = forensic->node_children("ForensicMarkFlag");
	BOOST_REQUIRE_EQUAL (flags.size(), 1);
	BOOST_CHECK_EQUAL (flags.front()->content(), "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-picture-disable");
}

/** Check ForensicMarkFlagList handling: disable audio but not picture */
BOOST_AUTO_TEST_CASE (kdm_forensic_test3)
{
	cxml::Document doc;
	cxml::ConstNodePtr forensic = kdm_forensic_test(doc, 0, -1);
	BOOST_REQUIRE (forensic);
	list<cxml::NodePtr> flags = forensic->node_children("ForensicMarkFlag");
	BOOST_REQUIRE_EQUAL (flags.size(), 1);
	BOOST_CHECK_EQUAL (flags.front()->content(), "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-audio-disable");
}

/** Check ForensicMarkFlagList handling: disable picture and audio above channel 3 */
BOOST_AUTO_TEST_CASE (kdm_forensic_test4)
{
	cxml::Document doc;
	cxml::ConstNodePtr forensic = kdm_forensic_test(doc, -1, 3);
	BOOST_REQUIRE (forensic);
	list<cxml::NodePtr> flags = forensic->node_children("ForensicMarkFlag");
	BOOST_REQUIRE_EQUAL (flags.size(), 2);
	BOOST_CHECK_EQUAL (flags.front()->content(), "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-picture-disable");
	BOOST_CHECK_EQUAL (flags.back()->content(), "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-audio-disable-above-channel-3");
}

/** Check ForensicMarkFlagList handling: disable neither */
BOOST_AUTO_TEST_CASE (kdm_forensic_test5)
{
	cxml::Document doc;
	cxml::ConstNodePtr forensic = kdm_forensic_test(doc, 0, 0);
	BOOST_CHECK (!forensic);
}
