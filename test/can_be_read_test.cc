/*
    Copyright (C) 2025 Carl Hetherington <cth@carlh.net>

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


#include "encrypted_kdm.h"
#include "mono_j2k_picture_asset.h"
#include "reel_atmos_asset.h"
#include "reel_mono_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_text_asset.h"
#include <libcxml/cxml.h>
#include <boost/test/unit_test.hpp>
#include "test.h"


using std::function;
using std::shared_ptr;


BOOST_AUTO_TEST_CASE(can_be_read_in_reel_mono_picture_asset_unencrypted)
{
	cxml::Document doc("Dummy");
	doc.read_string(
		"<Dummy>"
		"<MainPicture>"
		  "<Id>urn:uuid:5407b210-4441-4e97-8b16-8bdc7c12da54</Id>"
		  "<EditRate>25 1</EditRate>"
		  "<IntrinsicDuration>2508</IntrinsicDuration>"
		  "<EntryPoint>225</EntryPoint>"
		  "<Duration>2283</Duration>"
		  "<Hash>hcE3Lb8IEDIre9qXNEt64Z5RcNw=</Hash>"
		  "<FrameRate>25 1</FrameRate>"
		  "<ScreenAspectRatio>1998 1080</ScreenAspectRatio>"
		"</MainPicture>"
		"</Dummy>"
	);

	dcp::ReelMonoPictureAsset reel_asset(doc.node_child("MainPicture"));

	/* Unresolved */
	BOOST_CHECK(!reel_asset.can_be_read());

	auto asset = std::make_shared<dcp::MonoJ2KPictureAsset>("test/data/DCP/video.mxf");
	reel_asset.asset_ref().resolve({asset});

	/* Resolved */
	BOOST_CHECK(reel_asset.can_be_read());
}


BOOST_AUTO_TEST_CASE(can_be_read_in_reel_sound_asset_unencrypted)
{
	cxml::Document doc("Dummy");
	doc.read_string(
		"<Dummy>"
		"<MainSound>"
		  "<Id>urn:uuid:97f0f352-5b77-48ee-a558-9df37717f4fa</Id>"
		  "<EditRate>25 1</EditRate>"
		  "<IntrinsicDuration>2508</IntrinsicDuration>"
		  "<EntryPoint>225</EntryPoint>"
		  "<Duration>2283</Duration>"
		  "<Hash>hcE3Lb8IEDIre9qXNEt64Z5RcNw=</Hash>"
		"</MainSound>"
		"</Dummy>"
	);

	dcp::ReelSoundAsset reel_asset(doc.node_child("MainSound"));

	/* Unresolved */
	BOOST_CHECK(!reel_asset.can_be_read());

	auto asset = std::make_shared<dcp::SoundAsset>("test/data/DCP/audio.mxf");
	reel_asset.asset_ref().resolve({asset});

	/* Resolved */
	BOOST_CHECK(reel_asset.can_be_read());
}


static void
can_be_read_in_reel_encrypted_one(function<shared_ptr<dcp::ReelFileAsset> (shared_ptr<const dcp::Reel>)> get_asset)
{
	auto dcp = dcp::DCP(private_test / "data" / "encrypted_dcp_with_subs_and_atmos");
	dcp.read();

	BOOST_REQUIRE_EQUAL(dcp.cpls().size(), 1U);
	BOOST_REQUIRE_EQUAL(dcp.cpls()[0]->reels().size(), 1U);
	auto reel_asset = get_asset(dcp.cpls()[0]->reels()[0]);

	/* Encrypted */
	BOOST_CHECK(!reel_asset->can_be_read());

	dcp::DecryptedKDM wrong_kdm(
		dcp::EncryptedKDM(
			dcp::file_to_string("test/data/other_kdm.xml")
			),
		dcp::file_to_string("test/data/private.key")
		);
	dcp.add(wrong_kdm);

	/* Wrong KDM */
	BOOST_CHECK(!reel_asset->can_be_read());

	dcp::DecryptedKDM right_kdm(
		dcp::EncryptedKDM(dcp::file_to_string(private_test / "encrypted_dcp_with_subs_and_atmos.xml")),
		dcp::file_to_string("test/data/private.key")
		);
	dcp.add(right_kdm);

	/* Right KDM */
	BOOST_CHECK(reel_asset->can_be_read());
}


BOOST_AUTO_TEST_CASE(can_be_read_in_reel_encrypted)
{
	can_be_read_in_reel_encrypted_one([](shared_ptr<const dcp::Reel> reel) { return reel->main_picture(); });
	can_be_read_in_reel_encrypted_one([](shared_ptr<const dcp::Reel> reel) { return reel->main_sound(); });
	can_be_read_in_reel_encrypted_one([](shared_ptr<const dcp::Reel> reel) { return reel->main_subtitle(); });
	can_be_read_in_reel_encrypted_one([](shared_ptr<const dcp::Reel> reel) { return reel->atmos(); });
}

