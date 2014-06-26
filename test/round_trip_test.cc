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

#include "certificates.h"
#include "decrypted_kdm.h"
#include "encrypted_kdm.h"
#include "signer.h"
#include "mono_picture_mxf.h"
#include "sound_mxf.h"
#include "reel.h"
#include "test.h"
#include "cpl.h"
#include "mono_picture_frame.h"
#include "argb_frame.h"
#include "signer_chain.h"
#include "mono_picture_mxf_writer.h"
#include "reel_picture_asset.h"
#include "reel_mono_picture_asset.h"
#include "file.h"
#include <boost/test/unit_test.hpp>
#include <iostream>

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

	shared_ptr<dcp::MonoPictureMXF> mxf_A (new dcp::MonoPictureMXF (dcp::Fraction (24, 1)));
	shared_ptr<dcp::PictureMXFWriter> writer = mxf_A->start_write (work_dir / "video.mxf", dcp::SMPTE, false);
	dcp::File j2c ("test/data/32x32_red_square.j2c");
	for (int i = 0; i < 24; ++i) {
		writer->write (j2c.data (), j2c.size ());
	}
	writer->finalize ();

	dcp::Key key;

	mxf_A->set_key (key);

	shared_ptr<dcp::CPL> cpl (new dcp::CPL ("A Test DCP", dcp::FEATURE));
	shared_ptr<dcp::Reel> reel (new dcp::Reel ());
	reel->add (shared_ptr<dcp::ReelMonoPictureAsset> (new dcp::ReelMonoPictureAsset (mxf_A, 0)));
	cpl->add (reel);

	/* A KDM using our certificate chain's leaf key pair */
	dcp::DecryptedKDM kdm_A (
		cpl,
		dcp::LocalTime ("2013-01-01T00:00:00+00:00"),
		dcp::LocalTime ("2013-01-08T00:00:00+00:00"),
		"libdcp",
		"test",
		"2012-07-17T04:45:18+00:00"
		);

	boost::filesystem::path const kdm_file = work_dir / "kdm.xml";

	kdm_A.encrypt(signer, signer->certificates().leaf(), dcp::MODIFIED_TRANSITIONAL_1).as_xml (kdm_file);

	/* Reload the KDM, using our private key to decrypt it */
	dcp::DecryptedKDM kdm_B (dcp::EncryptedKDM (kdm_file), "build/test/signer/leaf.key");

	/* Check that the decrypted KDMKeys are the same as the ones we started with */
	BOOST_CHECK_EQUAL (kdm_A.keys().size(), kdm_B.keys().size());
	list<dcp::DecryptedKDMKey> keys_A = kdm_A.keys ();
	list<dcp::DecryptedKDMKey> keys_B = kdm_B.keys ();
	list<dcp::DecryptedKDMKey>::const_iterator i = keys_A.begin();
	list<dcp::DecryptedKDMKey>::const_iterator j = keys_B.begin();
	while (i != keys_A.end ()) {
		BOOST_CHECK (*i == *j);
		++i;
		++j;
	}

	/* Reload the picture MXF */
	shared_ptr<dcp::MonoPictureMXF> mxf_B (
		new dcp::MonoPictureMXF (work_dir / "video.mxf")
		);

	BOOST_CHECK (!kdm_B.keys().empty ());
	mxf_B->set_key (kdm_B.keys().front().key());

	shared_ptr<dcp::ARGBFrame> frame_A = mxf_A->get_frame(0)->argb_frame ();
	shared_ptr<dcp::ARGBFrame> frame_B = mxf_B->get_frame(0)->argb_frame ();
	BOOST_CHECK_EQUAL (frame_A->size().width, frame_B->size().width);
	BOOST_CHECK_EQUAL (frame_A->size().height, frame_B->size().height);
	BOOST_CHECK_EQUAL (memcmp (frame_A->data(), frame_B->data(), frame_A->size().width * frame_A->size().height), 0);
}
