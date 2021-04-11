/*
    Copyright (C) 2013-2019 Carl Hetherington <cth@carlh.net>

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

#include "metadata.h"
#include "certificate.h"
#include "dcp.h"
#include "certificate_chain.h"
#include "cpl.h"
#include "mono_picture_asset.h"
#include "picture_asset_writer.h"
#include "sound_asset_writer.h"
#include "sound_asset.h"
#include "reel.h"
#include "test.h"
#include "subtitle_asset.h"
#include "reel_mono_picture_asset.h"
#include "reel_sound_asset.h"
#include "encrypted_kdm.h"
#include "decrypted_kdm.h"
#include <asdcp/KM_util.h>
#include <sndfile.h>
#include <boost/test/unit_test.hpp>
#include <memory>


using std::vector;
using std::string;
using std::shared_ptr;
using std::make_shared;


/** Load a certificate chain from build/test/data/ *.pem and then build
 *  an encrypted DCP and a KDM using it.
 */
BOOST_AUTO_TEST_CASE (encryption_test)
{
	boost::filesystem::remove_all ("build/test/signer");
	boost::filesystem::create_directory ("build/test/signer");

	RNGFixer fix;

	dcp::MXFMetadata mxf_metadata;
	mxf_metadata.company_name = "OpenDCP";
	mxf_metadata.product_name = "OpenDCP";
	mxf_metadata.product_version = "0.0.25";

	boost::filesystem::remove_all ("build/test/DCP/encryption_test");
	boost::filesystem::create_directories ("build/test/DCP/encryption_test");
	dcp::DCP d ("build/test/DCP/encryption_test");

	/* Use test/ref/crypt so this test is repeatable */
	auto signer = make_shared<dcp::CertificateChain>();
	signer->add (dcp::Certificate(dcp::file_to_string("test/ref/crypt/ca.self-signed.pem")));
	signer->add (dcp::Certificate(dcp::file_to_string("test/ref/crypt/intermediate.signed.pem")));
	signer->add (dcp::Certificate(dcp::file_to_string("test/ref/crypt/leaf.signed.pem")));
	signer->set_key (dcp::file_to_string("test/ref/crypt/leaf.key"));

	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);

	dcp::Key key;

	auto mp = make_shared<dcp::MonoPictureAsset>(dcp::Fraction (24, 1), dcp::Standard::SMPTE);
	mp->set_metadata (mxf_metadata);
	mp->set_key (key);

	auto writer = mp->start_write ("build/test/DCP/encryption_test/video.mxf", false);
	dcp::ArrayData j2c ("test/data/flat_red.j2c");
	for (int i = 0; i < 24; ++i) {
		writer->write (j2c.data (), j2c.size ());
	}
	writer->finalize ();

	auto ms = make_shared<dcp::SoundAsset>(dcp::Fraction (24, 1), 48000, 1, dcp::LanguageTag("en-GB"), dcp::Standard::SMPTE);
	ms->set_metadata (mxf_metadata);
	ms->set_key (key);
	auto sound_writer = ms->start_write ("build/test/DCP/encryption_test/audio.mxf");

	SF_INFO info;
	info.format = 0;
	auto sndfile = sf_open ("test/data/1s_24-bit_48k_silence.wav", SFM_READ, &info);
	BOOST_CHECK (sndfile);
	float buffer[4096*6];
	float* channels[1];
	channels[0] = buffer;
	while (true) {
		auto N = sf_readf_float (sndfile, buffer, 4096);
		sound_writer->write (channels, N);
		if (N < 4096) {
			break;
		}
	}

	sound_writer->finalize ();

	cpl->add (make_shared<dcp::Reel>(
			make_shared<dcp::ReelMonoPictureAsset>(mp, 0),
			make_shared<dcp::ReelSoundAsset>(ms, 0),
			shared_ptr<dcp::ReelSubtitleAsset>()
			));
	cpl->set_content_version (
		dcp::ContentVersion("urn:uri:81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00", "81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00")
		);
	cpl->set_annotation_text ("A Test DCP");
	cpl->set_issuer ("OpenDCP 0.0.25");
	cpl->set_creator ("OpenDCP 0.0.25");
	cpl->set_issue_date ("2012-07-17T04:45:18+00:00");

	d.add (cpl);

	d.write_xml ("OpenDCP 0.0.25", "OpenDCP 0.0.25", "2012-07-17T04:45:18+00:00", "Created by libdcp", signer);

	dcp::DecryptedKDM kdm (
		cpl,
		key,
		dcp::LocalTime ("2016-01-01T00:00:00+00:00"),
		dcp::LocalTime ("2017-01-08T00:00:00+00:00"),
		"libdcp",
		"test",
		"2012-07-17T04:45:18+00:00"
		);

	kdm.encrypt (signer, signer->leaf(), vector<string>(), dcp::Formulation::MODIFIED_TRANSITIONAL_1, true, 0).as_xml("build/test/encryption_test.kdm.xml");

	int r = system (
		"xmllint --path schema --nonet --noout --schema schema/SMPTE-430-1-2006-Amd-1-2009-KDM.xsd build/test/encryption_test.kdm.xml "
		"> build/test/xmllint.log 2>&1 < /dev/null"
		);

#ifdef LIBDCP_WINDOWS
	BOOST_CHECK_EQUAL (r, 0);
#else
	BOOST_CHECK_EQUAL (WEXITSTATUS (r), 0);
#endif

	r = system ("xmlsec1 verify "
		"--pubkey-cert-pem test/ref/crypt/leaf.signed.pem "
		"--trusted-pem test/ref/crypt/intermediate.signed.pem "
		"--trusted-pem test/ref/crypt/ca.self-signed.pem "
		"--id-attr:Id http://www.smpte-ra.org/schemas/430-3/2006/ETM:AuthenticatedPublic "
		"--id-attr:Id http://www.smpte-ra.org/schemas/430-3/2006/ETM:AuthenticatedPrivate "
		    "build/test/encryption_test.kdm.xml > build/test/xmlsec1.log 2>&1 < /dev/null");

#ifdef LIBDCP_WINDOWS
	BOOST_CHECK_EQUAL (r, 0);
#else
	BOOST_CHECK_EQUAL (WEXITSTATUS (r), 0);
#endif
}
