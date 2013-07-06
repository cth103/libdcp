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

#include <tiffio.h>
#include "kdm.h"
#include "picture_frame.h"
#include "argb_frame.h"

using boost::dynamic_pointer_cast;

BOOST_AUTO_TEST_CASE (decryption_test)
{
	boost::filesystem::path plaintext_path = test_corpus;
	plaintext_path /= "TONEPLATES-SMPTE-PLAINTEXT_TST_F_XX-XX_ITL-TD_51-XX_2K_WOE_20111001_WOE_OV";
	libdcp::DCP plaintext (plaintext_path.string ());
	plaintext.read ();
	BOOST_CHECK_EQUAL (plaintext.encrypted (), false);

	boost::filesystem::path encrypted_path = test_corpus;
	encrypted_path /= "TONEPLATES-SMPTE-ENCRYPTED_TST_F_XX-XX_ITL-TD_51-XX_2K_WOE_20111001_WOE_OV";
	libdcp::DCP encrypted (encrypted_path.string ());
	encrypted.read ();
	BOOST_CHECK_EQUAL (encrypted.encrypted (), true);

	libdcp::KDM kdm (
		"test/data/kdm_TONEPLATES-SMPTE-ENC_.smpte-430-2.ROOT.NOT_FOR_PRODUCTION_20130706_20230702_CAR_OV_t1_8971c838.xml",
		"test/data/private.key"
		);

	encrypted.add_kdm (kdm);

	shared_ptr<const libdcp::Reel> encrypted_reel = encrypted.cpls().front()->reels().front ();
	shared_ptr<const libdcp::PictureAsset> encrypted_picture = encrypted_reel->main_picture ();
	BOOST_CHECK (encrypted_picture);

	shared_ptr<const libdcp::MonoPictureAsset> encrypted_mono_picture = dynamic_pointer_cast<const libdcp::MonoPictureAsset> (encrypted_picture);
	shared_ptr<const libdcp::MonoPictureFrame> j2k_frame = encrypted_mono_picture->get_frame (0);

	shared_ptr<const libdcp::ARGBFrame> argb_frame = j2k_frame->argb_frame ();



	uint8_t* tiff_frame = new uint8_t[1998 * 3 * 1080];
	
	uint8_t* t = tiff_frame;
	uint8_t* r = argb_frame->data ();
	for (int y = 0; y < 1080; ++y) {
		for (int x = 0; x < 1998; ++x) {
			/* Our data is first-byte blue, second-byte green, third-byte red, fourth-byte alpha,
			   so we need to twiddle here.
			*/
			
			t[0] = r[2]; // red
			t[1] = r[1]; // green
			t[2] = r[0]; // blue
			t += 3;
			r += 4;
		}
	}
	
	TIFF* output = TIFFOpen ("foo.tiff", "w");
	BOOST_CHECK (output);
	
	TIFFSetField (output, TIFFTAG_IMAGEWIDTH, 1998);
	TIFFSetField (output, TIFFTAG_IMAGELENGTH, 1080);
	TIFFSetField (output, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	TIFFSetField (output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField (output, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	TIFFSetField (output, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField (output, TIFFTAG_SAMPLESPERPIXEL, 3);
	
	BOOST_CHECK (TIFFWriteEncodedStrip (output, 0, tiff_frame, 1998 * 1080 * 3));
	TIFFClose (output);

	delete[] tiff_frame;
}
