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

#include "dcp.h"
#include "mono_picture_frame.h"
#include "cpl.h"
#include "decrypted_kdm.h"
#include "encrypted_kdm.h"
#include "mono_picture_asset.h"
#include "mono_picture_asset_reader.h"
#include "reel_picture_asset.h"
#include "reel.h"
#include "test.h"
#include "openjpeg_image.h"
#include "rgb_xyz.h"
#include "colour_conversion.h"
#include "stream_operators.h"
#include <boost/test/unit_test.hpp>
#include <boost/scoped_array.hpp>

using std::pair;
using std::make_pair;
using std::dynamic_pointer_cast;
using std::shared_ptr;
using boost::scoped_array;

pair<uint8_t*, dcp::Size>
get_frame (dcp::DCP const & dcp)
{
	shared_ptr<const dcp::Reel> reel = dcp.cpls().front()->reels().front ();
	shared_ptr<dcp::PictureAsset> picture = reel->main_picture()->asset ();
	BOOST_CHECK (picture);

	shared_ptr<const dcp::MonoPictureAsset> mono_picture = dynamic_pointer_cast<const dcp::MonoPictureAsset> (picture);
	shared_ptr<const dcp::MonoPictureAssetReader> reader = mono_picture->start_read ();
	shared_ptr<const dcp::MonoPictureFrame> j2k_frame = reader->get_frame (0);
	shared_ptr<dcp::OpenJPEGImage> xyz = j2k_frame->xyz_image();

	uint8_t* argb = new uint8_t[xyz->size().width * xyz->size().height * 4];
	dcp::xyz_to_rgba (j2k_frame->xyz_image(), dcp::ColourConversion::srgb_to_xyz(), argb, xyz->size().width * 4);
	return make_pair (argb, xyz->size ());
}

/** Decrypt an encrypted test DCP and check that its first frame is the same as the unencrypted version */
BOOST_AUTO_TEST_CASE (decryption_test)
{
	boost::filesystem::path plaintext_path = private_test;
	plaintext_path /= "TONEPLATES-SMPTE-PLAINTEXT_TST_F_XX-XX_ITL-TD_51-XX_2K_WOE_20111001_WOE_OV";
	dcp::DCP plaintext (plaintext_path.string ());
	plaintext.read ();
	BOOST_CHECK_EQUAL (plaintext.any_encrypted(), false);

	boost::filesystem::path encrypted_path = private_test;
	encrypted_path /= "TONEPLATES-SMPTE-ENCRYPTED_TST_F_XX-XX_ITL-TD_51-XX_2K_WOE_20111001_WOE_OV";
	dcp::DCP encrypted (encrypted_path.string ());
	encrypted.read ();
	BOOST_CHECK_EQUAL (encrypted.any_encrypted(), true);

	dcp::DecryptedKDM kdm (
		dcp::EncryptedKDM (
			dcp::file_to_string ("test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml")
			),
		dcp::file_to_string ("test/data/private.key")
		);

	encrypted.add (kdm);

	pair<uint8_t *, dcp::Size> plaintext_frame = get_frame (plaintext);
	pair<uint8_t *, dcp::Size> encrypted_frame = get_frame (encrypted);

	/* Check that plaintext and encrypted are the same */

	BOOST_CHECK_EQUAL (plaintext_frame.second, encrypted_frame.second);

	BOOST_CHECK_EQUAL (
		memcmp (
			plaintext_frame.first,
			encrypted_frame.first,
			plaintext_frame.second.width * plaintext_frame.second.height * 4
			),
		0
		);

	delete[] plaintext_frame.first;
	delete[] encrypted_frame.first;
}

/** Load in a KDM that didn't work at first */
BOOST_AUTO_TEST_CASE (failing_kdm_test)
{
	dcp::DecryptedKDM kdm (
		dcp::EncryptedKDM (dcp::file_to_string ("test/data/target.pem.crt.de5d4eba-e683-41ca-bdda-aa4ad96af3f4.kdm.xml")),
		dcp::file_to_string ("test/data/private.key")
		);
}
