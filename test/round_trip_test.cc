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

#include <iostream>
#include <boost/test/unit_test.hpp>
#include "certificates.h"
#include "kdm.h"
#include "signer.h"
#include "mono_picture_asset.h"
#include "sound_asset.h"
#include "reel.h"
#include "test.h"
#include "cpl.h"
#include "mono_picture_frame.h"
#include "argb_frame.h"
#include "signer_chain.h"

using std::list;
using boost::shared_ptr;

/* Build an encrypted picture MXF and a KDM for it and check that the KDM can be decrypted */
BOOST_AUTO_TEST_CASE (round_trip_test)
{
	boost::filesystem::remove_all ("build/test/signer");
	boost::filesystem::create_directory ("build/test/signer");
	libdcp::make_signer_chain ("build/test/signer", "openssl");
	
	libdcp::CertificateChain chain;
	chain.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate (boost::filesystem::path ("build/test/signer/ca.self-signed.pem"))));
	chain.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate (boost::filesystem::path ("build/test/signer/intermediate.signed.pem"))));
	chain.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate (boost::filesystem::path ("build/test/signer/leaf.signed.pem"))));

	shared_ptr<libdcp::Signer> signer (
		new libdcp::Signer (
			chain,
			"test/data/signer.key"
			)
		);

	boost::filesystem::path work_dir = "build/test/round_trip_test";
	boost::filesystem::create_directory (work_dir);

	shared_ptr<libdcp::MonoPictureAsset> asset_A (new libdcp::MonoPictureAsset (work_dir, "video.mxf"));
	asset_A->set_edit_rate (24);
	asset_A->set_intrinsic_duration (24);
	asset_A->set_size (libdcp::Size (32, 32));
	asset_A->create (j2c);

	libdcp::Key key;

	asset_A->set_key (key);

	shared_ptr<libdcp::CPL> cpl (new libdcp::CPL (work_dir, "A Test DCP", libdcp::FEATURE, 24, 24));
	cpl->add_reel (shared_ptr<libdcp::Reel> (new libdcp::Reel (asset_A, shared_ptr<libdcp::SoundAsset> (), shared_ptr<libdcp::SubtitleAsset> ())));
	libdcp::XMLMetadata metadata;
	cpl->write_xml (true, metadata, signer);

	boost::filesystem::path cpl_path = work_dir / (cpl->id() + "_cpl.xml");

	/* A KDM using our certificate chain's leaf key pair */
	libdcp::KDM kdm_A (
		cpl_path,
		signer,
		signer->certificates().leaf(),
		key,
		boost::posix_time::time_from_string ("2013-01-01 00:00:00"),
		boost::posix_time::time_from_string ("2013-01-08 00:00:00"),
		"libdcp",
		"2012-07-17T04:45:18+00:00",
		libdcp::KDM::MODIFIED_TRANSITIONAL_1
		);

	boost::filesystem::path const kdm_file = work_dir / "kdm.xml";

	kdm_A.as_xml (kdm_file);

	/* Reload the KDM, using our private key to decrypt it */
	libdcp::KDM kdm_B (kdm_file, "build/test/signer/leaf.key");

	/* Check that the decrypted KDMKeys are the same as the ones we started with */
	BOOST_CHECK_EQUAL (kdm_A.keys().size(), kdm_B.keys().size());
	list<libdcp::KDMKey> keys_A = kdm_A.keys ();
	list<libdcp::KDMKey> keys_B = kdm_B.keys ();
	list<libdcp::KDMKey>::const_iterator i = keys_A.begin();
	list<libdcp::KDMKey>::const_iterator j = keys_B.begin();
	while (i != keys_A.end ()) {
		BOOST_CHECK (*i == *j);
		++i;
		++j;
	}

	/* Reload the picture MXF */
	shared_ptr<libdcp::MonoPictureAsset> asset_B (
		new libdcp::MonoPictureAsset (work_dir, "video.mxf")
		);

	asset_B->set_key (kdm_B.keys().front().key());

	shared_ptr<libdcp::ARGBFrame> frame_A = asset_A->get_frame(0)->argb_frame ();
	shared_ptr<libdcp::ARGBFrame> frame_B = asset_B->get_frame(0)->argb_frame ();
	BOOST_CHECK_EQUAL (frame_A->size().width, frame_B->size().width);
	BOOST_CHECK_EQUAL (frame_A->size().height, frame_B->size().height);
	BOOST_CHECK_EQUAL (memcmp (frame_A->data(), frame_B->data(), frame_A->size().width * frame_A->size().height), 0);
}
