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
#include "mono_picture_mxf.h"
#include "sound_mxf.h"
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
	dcp::make_signer_chain ("build/test/signer", "openssl");
	
	dcp::CertificateChain chain;
	chain.add (shared_ptr<dcp::Certificate> (new dcp::Certificate (boost::filesystem::path ("build/test/signer/ca.self-signed.pem"))));
	chain.add (shared_ptr<dcp::Certificate> (new dcp::Certificate (boost::filesystem::path ("build/test/signer/intermediate.signed.pem"))));
	chain.add (shared_ptr<dcp::Certificate> (new dcp::Certificate (boost::filesystem::path ("build/test/signer/leaf.signed.pem"))));

	shared_ptr<dcp::Signer> signer (
		new dcp::Signer (
			chain,
			"test/data/signer.key"
			)
		);

	boost::filesystem::path work_dir = "build/test/round_trip_test";
	boost::filesystem::create_directory (work_dir);

	shared_ptr<dcp::MonoPictureMXF> asset_A (new dcp::MonoPictureMXF (work_dir, "video.mxf"));
	asset_A->set_edit_rate (24);
	asset_A->set_intrinsic_duration (24);
	asset_A->set_size (dcp::Size (32, 32));
	asset_A->create (j2c);

	dcp::Key key;

	asset_A->set_key (key);

	shared_ptr<dcp::CPL> cpl (new dcp::CPL (work_dir, "A Test DCP", dcp::FEATURE, 24, 24));
	cpl->add_reel (shared_ptr<dcp::Reel> (new dcp::Reel (asset_A, shared_ptr<dcp::SoundMXF> (), shared_ptr<dcp::SubtitleAsset> ())));

	/* A KDM using our certificate chain's leaf key pair */
	dcp::KDM kdm_A (
		cpl,
		signer,
		signer->certificates().leaf(),
		boost::posix_time::time_from_string ("2013-01-01 00:00:00"),
		boost::posix_time::time_from_string ("2013-01-08 00:00:00"),
		"libdcp",
		"2012-07-17T04:45:18+00:00"
		);

	boost::filesystem::path const kdm_file = work_dir / "kdm.xml";

	kdm_A.as_xml (kdm_file);

	/* Reload the KDM, using our private key to decrypt it */
	dcp::KDM kdm_B (kdm_file, "build/test/signer/leaf.key");

	/* Check that the decrypted KDMKeys are the same as the ones we started with */
	BOOST_CHECK_EQUAL (kdm_A.keys().size(), kdm_B.keys().size());
	list<dcp::KDMKey> keys_A = kdm_A.keys ();
	list<dcp::KDMKey> keys_B = kdm_B.keys ();
	list<dcp::KDMKey>::const_iterator i = keys_A.begin();
	list<dcp::KDMKey>::const_iterator j = keys_B.begin();
	while (i != keys_A.end ()) {
		BOOST_CHECK (*i == *j);
		++i;
		++j;
	}

	/* Reload the picture MXF */
	shared_ptr<dcp::MonoPictureMXF> asset_B (
		new dcp::MonoPictureMXF (work_dir, "video.mxf")
		);

	asset_B->set_key (kdm_B.keys().front().key());

	shared_ptr<dcp::ARGBFrame> frame_A = asset_A->get_frame(0)->argb_frame ();
	shared_ptr<dcp::ARGBFrame> frame_B = asset_B->get_frame(0)->argb_frame ();
	BOOST_CHECK_EQUAL (frame_A->size().width, frame_B->size().width);
	BOOST_CHECK_EQUAL (frame_A->size().height, frame_B->size().height);
	BOOST_CHECK_EQUAL (memcmp (frame_A->data(), frame_B->data(), frame_A->size().width * frame_A->size().height), 0);
}
