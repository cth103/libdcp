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

#include <boost/test/unit_test.hpp>
#include "kdm.h"
#include "dcp.h"
#include "mono_picture_frame.h"
#include "cpl.h"
#include "argb_frame.h"
#include "mono_picture_asset.h"
#include "reel.h"
#include "test.h"

using boost::dynamic_pointer_cast;
using boost::shared_ptr;

static shared_ptr<const libdcp::ARGBFrame>
get_frame (libdcp::DCP const & dcp)
{
	shared_ptr<const libdcp::Reel> reel = dcp.cpls().front()->reels().front ();
	shared_ptr<const libdcp::PictureAsset> picture = reel->main_picture ();
	BOOST_CHECK (picture);

	shared_ptr<const libdcp::MonoPictureAsset> mono_picture = dynamic_pointer_cast<const libdcp::MonoPictureAsset> (picture);
	shared_ptr<const libdcp::MonoPictureFrame> j2k_frame = mono_picture->get_frame (0);
	return j2k_frame->argb_frame ();
}

/** Decrypt an encrypted test DCP and check that its first frame is the same as the unencrypted version */
BOOST_AUTO_TEST_CASE (decryption_test)
{
	boost::filesystem::path plaintext_path = private_test;
	plaintext_path /= "TONEPLATES-SMPTE-PLAINTEXT_TST_F_XX-XX_ITL-TD_51-XX_2K_WOE_20111001_WOE_OV";
	libdcp::DCP plaintext (plaintext_path.string ());
	plaintext.read ();
	BOOST_CHECK_EQUAL (plaintext.encrypted (), false);

	boost::filesystem::path encrypted_path = private_test;
	encrypted_path /= "TONEPLATES-SMPTE-ENCRYPTED_TST_F_XX-XX_ITL-TD_51-XX_2K_WOE_20111001_WOE_OV";
	libdcp::DCP encrypted (encrypted_path.string ());
	encrypted.read ();
	BOOST_CHECK_EQUAL (encrypted.encrypted (), true);

	libdcp::KDM kdm (
		"test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml",
		"test/data/private.key"
		);

	encrypted.add_kdm (kdm);

	shared_ptr<const libdcp::ARGBFrame> plaintext_frame = get_frame (plaintext);
	shared_ptr<const libdcp::ARGBFrame> encrypted_frame = get_frame (encrypted);

	/* Check that plaintext and encrypted are the same */
	BOOST_CHECK_EQUAL (plaintext_frame->stride(), encrypted_frame->stride());
	BOOST_CHECK_EQUAL (plaintext_frame->size().width, encrypted_frame->size().width);
	BOOST_CHECK_EQUAL (plaintext_frame->size().height, encrypted_frame->size().height);
	BOOST_CHECK_EQUAL (memcmp (plaintext_frame->data(), encrypted_frame->data(), plaintext_frame->stride() * plaintext_frame->size().height), 0);
}

/** Load in a KDM that didn't work at first */
BOOST_AUTO_TEST_CASE (failing_kdm_test)
{
	libdcp::KDM kdm (
		"test/data/target.pem.crt.de5d4eba-e683-41ca-bdda-aa4ad96af3f4.kdm.xml",
		"test/data/private.key"
		);
}
