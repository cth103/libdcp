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

#include "certificate.h"
#include "decrypted_kdm.h"
#include "encrypted_kdm.h"
#include "certificate_chain.h"
#include "mono_picture_asset.h"
#include "sound_asset.h"
#include "reel.h"
#include "test.h"
#include "cpl.h"
#include "mono_picture_frame.h"
#include "certificate_chain.h"
#include "mono_picture_asset_writer.h"
#include "mono_picture_asset_reader.h"
#include "reel_picture_asset.h"
#include "reel_mono_picture_asset.h"
#include "openjpeg_image.h"
#include "rgb_xyz.h"
#include "colour_conversion.h"
#include <boost/test/unit_test.hpp>
#include <boost/scoped_array.hpp>
#include <iostream>


using std::list;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::vector;
using boost::scoped_array;


/** Build an encrypted picture asset and a KDM for it and check that the KDM can be decrypted */
BOOST_AUTO_TEST_CASE (round_trip_test)
{
	auto signer = make_shared<dcp::CertificateChain>(boost::filesystem::path ("openssl"));

	boost::filesystem::path work_dir = "build/test/round_trip_test";
	boost::filesystem::create_directory (work_dir);

	auto asset_A = make_shared<dcp::MonoPictureAsset>(dcp::Fraction (24, 1), dcp::Standard::SMPTE);
	auto writer = asset_A->start_write (work_dir / "video.mxf", false);
	dcp::ArrayData j2c ("test/data/flat_red.j2c");
	for (int i = 0; i < 24; ++i) {
		writer->write (j2c.data (), j2c.size ());
	}
	writer->finalize ();

	dcp::Key key;

	asset_A->set_key (key);

	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	auto reel = make_shared<dcp::Reel>();
	reel->add (make_shared<dcp::ReelMonoPictureAsset>(asset_A, 0));
	cpl->add (reel);

	dcp::LocalTime start;
	start.set_year (start.year() + 1);
	dcp::LocalTime end;
	end.set_year (end.year() + 2);

	/* A KDM using our certificate chain's leaf key pair */
	dcp::DecryptedKDM kdm_A (
		cpl,
		key,
		start,
		end,
		"libdcp",
		"test",
		"2012-07-17T04:45:18+00:00"
		);

	boost::filesystem::path const kdm_file = work_dir / "kdm.xml";

	kdm_A.encrypt(signer, signer->leaf(), vector<string>(), dcp::Formulation::MODIFIED_TRANSITIONAL_1, true, 0).as_xml (kdm_file);

	/* Reload the KDM, using our private key to decrypt it */
	dcp::DecryptedKDM kdm_B (dcp::EncryptedKDM (dcp::file_to_string (kdm_file)), signer->key().get ());

	/* Check that the decrypted KDMKeys are the same as the ones we started with */
	BOOST_CHECK_EQUAL (kdm_A.keys().size(), kdm_B.keys().size());
	auto keys_A = kdm_A.keys ();
	auto keys_B = kdm_B.keys ();
	auto i = keys_A.begin();
	auto j = keys_B.begin();
	while (i != keys_A.end ()) {
		BOOST_CHECK (*i == *j);
		++i;
		++j;
	}

	/* Reload the picture asset */
	auto asset_B = make_shared<dcp::MonoPictureAsset>(work_dir / "video.mxf");

	BOOST_CHECK (!kdm_B.keys().empty ());
	asset_B->set_key (kdm_B.keys().front().key());

	auto xyz_A = asset_A->start_read()->get_frame(0)->xyz_image ();
	auto xyz_B = asset_B->start_read()->get_frame(0)->xyz_image ();

	scoped_array<uint8_t> frame_A (new uint8_t[xyz_A->size().width * xyz_A->size().height * 4]);
	dcp::xyz_to_rgba (xyz_A, dcp::ColourConversion::srgb_to_xyz(), frame_A.get(), xyz_A->size().width * 4);

	scoped_array<uint8_t> frame_B (new uint8_t[xyz_B->size().width * xyz_B->size().height * 4]);
	dcp::xyz_to_rgba (xyz_B, dcp::ColourConversion::srgb_to_xyz(), frame_B.get(), xyz_B->size().width * 4);

	BOOST_CHECK_EQUAL (xyz_A->size().width, xyz_B->size().width);
	BOOST_CHECK_EQUAL (xyz_A->size().height, xyz_B->size().height);
	BOOST_CHECK_EQUAL (memcmp (frame_A.get(), frame_B.get(), xyz_A->size().width * xyz_A->size().height * 4), 0);
}
