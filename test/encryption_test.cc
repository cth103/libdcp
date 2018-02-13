/*
    Copyright (C) 2013-2015 Carl Hetherington <cth@carlh.net>

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
#include "file.h"
#include "subtitle_asset.h"
#include "reel_mono_picture_asset.h"
#include "reel_sound_asset.h"
#include "encrypted_kdm.h"
#include "decrypted_kdm.h"
#include <asdcp/KM_util.h>
#include <sndfile.h>
#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>

using std::vector;
using boost::shared_ptr;

/** Load a certificate chain from build/test/data/ *.pem and then build
 *  an encrypted DCP and a KDM using it.
 */
BOOST_AUTO_TEST_CASE (encryption_test)
{
	boost::filesystem::remove_all ("build/test/signer");
	boost::filesystem::create_directory ("build/test/signer");

	Kumu::cth_test = true;

	dcp::MXFMetadata mxf_metadata;
	mxf_metadata.company_name = "OpenDCP";
	mxf_metadata.product_name = "OpenDCP";
	mxf_metadata.product_version = "0.0.25";

	dcp::XMLMetadata xml_metadata;
	xml_metadata.annotation_text = "A Test DCP";
	xml_metadata.issuer = "OpenDCP 0.0.25";
	xml_metadata.creator = "OpenDCP 0.0.25";
	xml_metadata.issue_date = "2012-07-17T04:45:18+00:00";

	boost::filesystem::remove_all ("build/test/DCP/encryption_test");
	boost::filesystem::create_directories ("build/test/DCP/encryption_test");
	dcp::DCP d ("build/test/DCP/encryption_test");

	/* Use test/ref/crypt so this test is repeatable */
	shared_ptr<dcp::CertificateChain> signer (new dcp::CertificateChain ());
	signer->add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	signer->add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	signer->add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	signer->set_key (dcp::file_to_string ("test/ref/crypt/leaf.key"));

	shared_ptr<dcp::CPL> cpl (new dcp::CPL ("A Test DCP", dcp::FEATURE));

	dcp::Key key;

	shared_ptr<dcp::MonoPictureAsset> mp (new dcp::MonoPictureAsset (dcp::Fraction (24, 1)));
	mp->set_metadata (mxf_metadata);
	mp->set_key (key);

	shared_ptr<dcp::PictureAssetWriter> writer = mp->start_write ("build/test/DCP/encryption_test/video.mxf", dcp::SMPTE, false);
	dcp::File j2c ("test/data/32x32_red_square.j2c");
	for (int i = 0; i < 24; ++i) {
		writer->write (j2c.data (), j2c.size ());
	}
	writer->finalize ();

	shared_ptr<dcp::SoundAsset> ms (new dcp::SoundAsset (dcp::Fraction (24, 1), 48000, 1));
	ms->set_metadata (mxf_metadata);
	ms->set_key (key);
	shared_ptr<dcp::SoundAssetWriter> sound_writer = ms->start_write ("build/test/DCP/encryption_test/audio.mxf", dcp::SMPTE);

	SF_INFO info;
	info.format = 0;
	SNDFILE* sndfile = sf_open ("test/data/1s_24-bit_48k_silence.wav", SFM_READ, &info);
	BOOST_CHECK (sndfile);
	float buffer[4096*6];
	float* channels[1];
	channels[0] = buffer;
	while (1) {
		sf_count_t N = sf_readf_float (sndfile, buffer, 4096);
		sound_writer->write (channels, N);
		if (N < 4096) {
			break;
		}
	}

	sound_writer->finalize ();

	cpl->add (shared_ptr<dcp::Reel> (new dcp::Reel (
						 shared_ptr<dcp::ReelMonoPictureAsset> (new dcp::ReelMonoPictureAsset (mp, 0)),
						 shared_ptr<dcp::ReelSoundAsset> (new dcp::ReelSoundAsset (ms, 0)),
						 shared_ptr<dcp::ReelSubtitleAsset> ()
						 )));
	cpl->set_content_version_id ("urn:uri:81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00");
	cpl->set_content_version_label_text ("81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00");
	cpl->set_metadata (xml_metadata);

	d.add (cpl);

	xml_metadata.annotation_text = "Created by libdcp";
	d.write_xml (dcp::SMPTE, xml_metadata, signer);

	dcp::DecryptedKDM kdm (
		cpl,
		key,
		dcp::LocalTime ("2013-01-01T00:00:00+00:00"),
		dcp::LocalTime ("2017-01-08T00:00:00+00:00"),
		"libdcp",
		"test",
		"2012-07-17T04:45:18+00:00"
		);

	kdm.encrypt (signer, signer->leaf(), vector<dcp::Certificate>(), dcp::MODIFIED_TRANSITIONAL_1, -1, -1).as_xml ("build/test/encryption_test.kdm.xml");

	int r = system (
		"xmllint --path schema --nonet --noout --schema schema/SMPTE-430-1-2006-Amd-1-2009-KDM.xsd build/test/encryption_test.kdm.xml "
		"> build/test/xmllint.log 2>&1 < /dev/null"
		);

#ifdef LIBDCP_POSIX
	BOOST_CHECK_EQUAL (WEXITSTATUS (r), 0);
#else
	BOOST_CHECK_EQUAL (r, 0);
#endif

	r = system ("xmlsec1 verify "
		"--pubkey-cert-pem test/ref/crypt/leaf.signed.pem "
		"--trusted-pem test/ref/crypt/intermediate.signed.pem "
		"--trusted-pem test/ref/crypt/ca.self-signed.pem "
		"--id-attr:Id http://www.smpte-ra.org/schemas/430-3/2006/ETM:AuthenticatedPublic "
		"--id-attr:Id http://www.smpte-ra.org/schemas/430-3/2006/ETM:AuthenticatedPrivate "
		    "build/test/encryption_test.kdm.xml > build/test/xmlsec1.log 2>&1 < /dev/null");

#ifdef LIBDCP_POSIX
	BOOST_CHECK_EQUAL (WEXITSTATUS (r), 0);
#else
	BOOST_CHECK_EQUAL (r, 0);
#endif
}
