/*
    Copyright (C) 2013-2015 Carl Hetherington <cth@carlh.net>

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
#include "certificate_chain.h"
#include "mono_picture_mxf_writer.h"
#include "reel_picture_asset.h"
#include "reel_mono_picture_asset.h"
#include "file.h"
#include "xyz_image.h"
#include "rgb_xyz.h"
#include "colour_conversion.h"
#include <boost/test/unit_test.hpp>
#include <boost/scoped_array.hpp>
#include <iostream>

using std::list;
using boost::shared_ptr;
using boost::scoped_array;

/* Build an encrypted picture MXF and a KDM for it and check that the KDM can be decrypted */
BOOST_AUTO_TEST_CASE (round_trip_test)
{
	shared_ptr<dcp::Signer> signer (new dcp::Signer ("openssl"));

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
		key,
		dcp::LocalTime ("2013-01-01T00:00:00+00:00"),
		dcp::LocalTime ("2013-01-08T00:00:00+00:00"),
		"libdcp",
		"test",
		"2012-07-17T04:45:18+00:00"
		);

	boost::filesystem::path const kdm_file = work_dir / "kdm.xml";

	kdm_A.encrypt(signer, signer->certificates().leaf(), dcp::MODIFIED_TRANSITIONAL_1).as_xml (kdm_file);

	/* Reload the KDM, using our private key to decrypt it */
	dcp::DecryptedKDM kdm_B (dcp::EncryptedKDM (dcp::file_to_string (kdm_file)), signer->key ());

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

	shared_ptr<dcp::XYZImage> xyz_A = mxf_A->get_frame(0)->xyz_image ();
	shared_ptr<dcp::XYZImage> xyz_B = mxf_B->get_frame(0)->xyz_image ();

	scoped_array<uint8_t> frame_A (new uint8_t[xyz_A->size().width * xyz_A->size().height * 4]);
	dcp::xyz_to_rgba (xyz_A, dcp::ColourConversion::xyz_to_srgb(), frame_A.get());
	
	scoped_array<uint8_t> frame_B (new uint8_t[xyz_B->size().width * xyz_B->size().height * 4]);
	dcp::xyz_to_rgba (xyz_B, dcp::ColourConversion::xyz_to_srgb(), frame_B.get());

	BOOST_CHECK_EQUAL (xyz_A->size().width, xyz_B->size().width);
	BOOST_CHECK_EQUAL (xyz_A->size().height, xyz_B->size().height);
	BOOST_CHECK_EQUAL (memcmp (frame_A.get(), frame_B.get(), xyz_A->size().width * xyz_A->size().height * 4), 0);
}
