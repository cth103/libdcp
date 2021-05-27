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

#include "colour_conversion.h"
#include "cpl.h"
#include "dcp.h"
#include "decrypted_kdm.h"
#include "encrypted_kdm.h"
#include "mono_picture_asset.h"
#include "mono_picture_asset_reader.h"
#include "mono_picture_frame.h"
#include "openjpeg_image.h"
#include "picture_asset_writer.h"
#include "reel.h"
#include "reel_file_asset.h"
#include "reel_mono_picture_asset.h"
#include "reel_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_smpte_subtitle_asset.h"
#include "rgb_xyz.h"
#include "smpte_subtitle_asset.h"
#include "sound_asset.h"
#include "sound_asset_writer.h"
#include "stream_operators.h"
#include "test.h"
#include <boost/test/unit_test.hpp>
#include <boost/scoped_array.hpp>


using std::dynamic_pointer_cast;
using std::make_pair;
using std::make_shared;
using std::map;
using std::pair;
using std::shared_ptr;
using std::string;
using boost::optional;
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
BOOST_AUTO_TEST_CASE (decryption_test1)
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


/** Make an encrypted SMPTE DCP with picture, sound and subs and check that the keys get distributed to the assets
 *  when we read it back in.
 */
BOOST_AUTO_TEST_CASE (decryption_test2)
{
	boost::filesystem::path dir = "build/test/decryption_test2";
	boost::filesystem::create_directory(dir);

	auto context_id = dcp::make_uuid();
	dcp::Key key;

	auto picture_asset = make_shared<dcp::MonoPictureAsset>(dcp::Fraction(24, 1), dcp::Standard::SMPTE);
	picture_asset->set_key (key);
	picture_asset->set_context_id (context_id);
	auto picture_writer = picture_asset->start_write(dir / "picture.mxf", false);
	dcp::ArrayData picture("test/data/flat_red.j2c");
	for (int i = 0; i < 24; ++i) {
		picture_writer->write(picture);
	}
	picture_writer->finalize();

	auto sound_asset = make_shared<dcp::SoundAsset>(dcp::Fraction(24, 1), 48000, 2, dcp::LanguageTag("en-GB"), dcp::Standard::SMPTE);
	sound_asset->set_key (key);
	sound_asset->set_context_id (context_id);
	auto sound_writer = sound_asset->start_write(dir / "sound.mxf");
	std::array<float, 48000> left;
	std::array<float, 48000> right;
	for (int i = 0; i < 48000; ++i) {
		left[i] = sin (2 * M_PI * i * 440 / 48000) * 0.25;
		right[i] = sin (2 * M_PI * i * 880 / 48000) * 0.25;
	}
	std::array<float*, 2> audio;
	audio[0] = left.data();
	audio[1] = right.data();
	sound_writer->write (audio.data(), 48000);
	sound_writer->finalize ();

	auto subs_asset = make_shared<dcp::SMPTESubtitleAsset>();
	subs_asset->set_key (key);
	subs_asset->set_context_id (context_id);
	subs_asset->add(make_shared<dcp::SubtitleString>(
		optional<string>(),
		false, false, false,
		dcp::Colour(255, 255, 255),
		42,
		1,
		dcp::Time(),
		dcp::Time(0, 0, 5, 0, 24),
		0.5, dcp::HAlign::CENTER,
		0.5, dcp::VAlign::CENTER,
		dcp::Direction::LTR,
		"Hello world",
		dcp::Effect::NONE,
		dcp::Colour(0, 0, 0),
		dcp::Time(), dcp::Time()
		));
	subs_asset->write (dir / "subs.mxf");

	auto reel = make_shared<dcp::Reel>();
	auto reel_picture_asset = make_shared<dcp::ReelMonoPictureAsset>(picture_asset, 0);
	auto reel_sound_asset = make_shared<dcp::ReelSoundAsset>(sound_asset, 0);
	auto reel_subs_asset = make_shared<dcp::ReelSMPTESubtitleAsset>(subs_asset, dcp::Fraction(24, 1), 120, 0);
	reel->add(reel_picture_asset);
	reel->add(reel_sound_asset);
	reel->add(reel_subs_asset);

	auto cpl = std::make_shared<dcp::CPL>("My film", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	cpl->add(reel);

	dcp::DCP dcp (dir);
	dcp.add (cpl);
	dcp.write_xml ();

	map<shared_ptr<const dcp::ReelFileAsset>, dcp::Key> keys;
	keys[reel_picture_asset] = key;
	keys[reel_sound_asset] = key;
	keys[reel_subs_asset] = key;

	dcp::DecryptedKDM kdm (cpl->id(), keys, dcp::LocalTime(), dcp::LocalTime(), "foo", "bar", "baz");

	dcp::DCP dcp_read (dir);
	dcp_read.read ();
	dcp_read.add (kdm);

	BOOST_REQUIRE_EQUAL (dcp_read.cpls().size(), 1U);
	auto cpl_read = dcp_read.cpls()[0];
	BOOST_REQUIRE_EQUAL (cpl_read->reels().size(), 1U);
	auto reel_read = cpl_read->reels()[0];

	BOOST_REQUIRE (reel_read->main_picture());
	BOOST_CHECK (reel_read->main_picture()->asset()->key());
	BOOST_REQUIRE (reel_read->main_sound());
	BOOST_CHECK (reel_read->main_sound()->asset()->key());
	BOOST_REQUIRE (reel_read->main_subtitle());
	auto smpte_sub = dynamic_pointer_cast<dcp::SMPTESubtitleAsset>(reel_read->main_subtitle()->asset());
	BOOST_REQUIRE (smpte_sub);
	BOOST_CHECK (smpte_sub->key());
}

