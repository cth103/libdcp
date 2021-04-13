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


#include "certificate_chain.h"
#include "cpl.h"
#include "decrypted_kdm.h"
#include "encrypted_kdm.h"
#include "mono_picture_asset.h"
#include "picture_asset_writer.h"
#include "reel.h"
#include "reel_mono_picture_asset.h"
#include "test.h"
#include "types.h"
#include "util.h"
#include "warnings.h"
#include <libcxml/cxml.h>
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <boost/test/unit_test.hpp>


using std::list;
using std::string;
using std::vector;
using std::make_shared;
using std::shared_ptr;
using boost::optional;


/** Check reading and decryption of a KDM */
BOOST_AUTO_TEST_CASE (kdm_test)
{
	dcp::DecryptedKDM kdm (
		dcp::EncryptedKDM (
			dcp::file_to_string ("test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml")
			),
		dcp::file_to_string ("test/data/private.key")
		);

	auto keys = kdm.keys ();

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

	auto parser = make_shared<xmlpp::DomParser>();
	parser->parse_memory (kdm.as_xml ());
	parser->get_document()->write_to_file_formatted ("build/kdm.xml", "UTF-8");
	check_xml (
		dcp::file_to_string("test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml"),
		dcp::file_to_string("build/kdm.xml"),
		{},
		true
		);
}


/** Test some of the utility methods of DecryptedKDM */
BOOST_AUTO_TEST_CASE (decrypted_kdm_test)
{
	auto data = new uint8_t[16];
	auto p = data;
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

	auto typed_key_ids = doc.node_child("AuthenticatedPublic")->
		node_child("RequiredExtensions")->
		node_child("KDMRequiredExtensions")->
		node_child("KeyIdList")->
		node_children("TypedKeyId");

	for (auto i: typed_key_ids) {
		for (auto j: i->node_children("KeyType")) {
			BOOST_CHECK (j->string_attribute("scope") == "http://www.smpte-ra.org/430-1/2006/KDM#kdm-key-type");
		}
	}
}


static cxml::ConstNodePtr
kdm_forensic_test (cxml::Document& doc, bool picture, optional<int> audio)
{
	dcp::DecryptedKDM decrypted (
		dcp::EncryptedKDM (
			dcp::file_to_string ("test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml")
			),
		dcp::file_to_string ("test/data/private.key")
		);

	auto signer = make_shared<dcp::CertificateChain>(dcp::file_to_string("test/data/certificate_chain"));
	signer->set_key(dcp::file_to_string("test/data/private.key"));

	dcp::EncryptedKDM kdm = decrypted.encrypt (
		signer, signer->leaf(), vector<string>(), dcp::Formulation::MODIFIED_TRANSITIONAL_1, picture, audio
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
	auto forensic = kdm_forensic_test(doc, true, 0);
	BOOST_REQUIRE (forensic);
	auto flags = forensic->node_children("ForensicMarkFlag");
	BOOST_REQUIRE_EQUAL (flags.size(), 2);
	BOOST_CHECK_EQUAL (flags.front()->content(), "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-picture-disable");
	BOOST_CHECK_EQUAL (flags.back()->content(), "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-audio-disable");
}


/** Check ForensicMarkFlagList handling: disable picture but not audio */
BOOST_AUTO_TEST_CASE (kdm_forensic_test2)
{
	cxml::Document doc;
	auto forensic = kdm_forensic_test(doc, true, optional<int>());
	BOOST_REQUIRE (forensic);
	auto flags = forensic->node_children("ForensicMarkFlag");
	BOOST_REQUIRE_EQUAL (flags.size(), 1);
	BOOST_CHECK_EQUAL (flags.front()->content(), "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-picture-disable");
}


/** Check ForensicMarkFlagList handling: disable audio but not picture */
BOOST_AUTO_TEST_CASE (kdm_forensic_test3)
{
	cxml::Document doc;
	auto forensic = kdm_forensic_test(doc, false, 0);
	BOOST_REQUIRE (forensic);
	auto flags = forensic->node_children("ForensicMarkFlag");
	BOOST_REQUIRE_EQUAL (flags.size(), 1);
	BOOST_CHECK_EQUAL (flags.front()->content(), "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-audio-disable");
}


/** Check ForensicMarkFlagList handling: disable picture and audio above channel 3 */
BOOST_AUTO_TEST_CASE (kdm_forensic_test4)
{
	cxml::Document doc;
	auto forensic = kdm_forensic_test(doc, true, 3);
	BOOST_REQUIRE (forensic);
	auto flags = forensic->node_children("ForensicMarkFlag");
	BOOST_REQUIRE_EQUAL (flags.size(), 2);
	BOOST_CHECK_EQUAL (flags.front()->content(), "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-picture-disable");
	BOOST_CHECK_EQUAL (flags.back()->content(), "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-audio-disable-above-channel-3");
}


/** Check ForensicMarkFlagList handling: disable neither */
BOOST_AUTO_TEST_CASE (kdm_forensic_test5)
{
	cxml::Document doc;
	auto forensic = kdm_forensic_test(doc, false, optional<int>());
	BOOST_CHECK (!forensic);
}


/** Check that KDM validity periods are checked for being within the certificate validity */
BOOST_AUTO_TEST_CASE (validity_period_test1)
{
	auto signer = make_shared<dcp::CertificateChain>(dcp::file_to_string("test/data/certificate_chain"));
	signer->set_key(dcp::file_to_string("test/data/private.key"));

	auto asset = make_shared<dcp::MonoPictureAsset>(dcp::Fraction(24, 1), dcp::Standard::SMPTE);
	asset->set_key (dcp::Key());
	auto writer = asset->start_write ("build/test/validity_period_test1.mxf", false);
	dcp::ArrayData frame ("test/data/flat_red.j2c");
	writer->write (frame.data(), frame.size());
	auto reel = make_shared<dcp::Reel>();
	reel->add(make_shared<dcp::ReelMonoPictureAsset>(asset, 0));
	auto cpl = make_shared<dcp::CPL>("test", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	cpl->add(reel);

	/* This certificate_chain is valid from 26/12/2012 to 24/12/2022 */

	/* Inside */
	BOOST_CHECK_NO_THROW(
		dcp::DecryptedKDM(
			cpl, dcp::Key(dcp::file_to_string("test/data/private.key")), dcp::LocalTime("2015-01-01T00:00:00"), dcp::LocalTime("2017-07-31T00:00:00"), "", "", ""
			).encrypt(signer, signer->leaf(), vector<string>(), dcp::Formulation::MODIFIED_TRANSITIONAL_1, true, optional<int>())
		);

	/* Starts too early */
	BOOST_CHECK_THROW(
		dcp::DecryptedKDM(
			cpl, dcp::Key(dcp::file_to_string("test/data/private.key")), dcp::LocalTime("1981-01-01T00:00:00"), dcp::LocalTime("2017-07-31T00:00:00"), "", "", ""
			).encrypt(signer, signer->leaf(), vector<string>(), dcp::Formulation::MODIFIED_TRANSITIONAL_1, true, optional<int>()),
		dcp::BadKDMDateError
		);

	/* Finishes too late */
	BOOST_CHECK_THROW(
		dcp::DecryptedKDM(
			cpl, dcp::Key(dcp::file_to_string("test/data/private.key")), dcp::LocalTime("2015-01-01T00:00:00"), dcp::LocalTime("2035-07-31T00:00:00"), "", "", ""
			).encrypt(signer, signer->leaf(), vector<string>(), dcp::Formulation::MODIFIED_TRANSITIONAL_1, true, optional<int>()),
		dcp::BadKDMDateError
		);

	/* Starts too early and finishes too late */
	BOOST_CHECK_THROW(
		dcp::DecryptedKDM(
			cpl, dcp::Key(dcp::file_to_string("test/data/private.key")), dcp::LocalTime("1981-01-01T00:00:00"), dcp::LocalTime("2035-07-31T00:00:00"), "", "", ""
			).encrypt(signer, signer->leaf(), vector<string>(), dcp::Formulation::MODIFIED_TRANSITIONAL_1, true, optional<int>()),
		dcp::BadKDMDateError
		);
}
