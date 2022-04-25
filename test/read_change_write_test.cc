/*
    Copyright (C) 2022 Carl Hetherington <cth@carlh.net>

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


#include "cpl.h"
#include "dcp.h"
#include "reel.h"
#include "reel_mono_picture_asset.h"
#include "reel_sound_asset.h"
#include "test.h"
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>


using std::make_shared;
using std::string;


/* Read a DCP, change a few things and write it back */
BOOST_AUTO_TEST_CASE(read_change_write_test)
{
	boost::filesystem::path path = "build/test/read_change_write_test";
	boost::filesystem::remove_all(path);
	boost::filesystem::create_directories(path);
	auto in_picture = simple_picture(path, "1");
	auto in_sound = simple_sound(path, "1", {}, "de-DE");
	auto in_reel_picture = make_shared<dcp::ReelMonoPictureAsset>(in_picture, 0);
	auto in_reel_sound = make_shared<dcp::ReelSoundAsset>(in_sound, 0);
	auto in_reel = make_shared<dcp::Reel>(in_reel_picture, in_reel_sound);
	auto in_cpl = make_shared<dcp::CPL>("Input CPL", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	in_cpl->add(in_reel);
	dcp::DCP in_dcp(path);
	in_dcp.add(in_cpl);
	in_dcp.set_issuer("my great issuer");
	in_dcp.set_creator("the creator");
	in_dcp.write_xml();

	dcp::DCP work_dcp(path);
	work_dcp.read();
	auto add_picture = simple_picture(path, "2");
	auto add_sound = simple_sound(path, "2", {}, "de-DE");
	auto add_reel_picture = make_shared<dcp::ReelMonoPictureAsset>(add_picture, 0);
	auto add_reel_sound = make_shared<dcp::ReelSoundAsset>(add_sound, 0);
	auto add_reel = make_shared<dcp::Reel>(add_reel_picture, add_reel_sound);
	auto add_cpl = make_shared<dcp::CPL>("Added CPL", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	add_cpl->add(add_reel);
	work_dcp.add(add_cpl);
	work_dcp.write_xml();

	auto id_in_xml = [](cxml::Document const& doc, string id) {
		auto assets = doc.node_child("AssetList")->node_children("Asset");
		return std::find_if(assets.begin(), assets.end(), [id](cxml::ConstNodePtr asset) {
			return dcp::remove_urn_uuid(asset->string_child("Id")) == id;
		}) != assets.end();
	};

	cxml::Document check_pkl("PackingList");
	check_pkl.read_file(find_file(path, "pkl_"));

	BOOST_CHECK_EQUAL(check_pkl.string_child("Issuer"), "my great issuer");
	BOOST_CHECK_EQUAL(check_pkl.string_child("Creator"), "the creator");

	BOOST_CHECK(id_in_xml(check_pkl, in_picture->id()));
	BOOST_CHECK(id_in_xml(check_pkl, in_sound->id()));
	BOOST_CHECK(id_in_xml(check_pkl, in_cpl->id()));
	BOOST_CHECK(id_in_xml(check_pkl, add_picture->id()));
	BOOST_CHECK(id_in_xml(check_pkl, add_sound->id()));
	BOOST_CHECK(id_in_xml(check_pkl, add_cpl->id()));

	auto const pkl_id = dcp::remove_urn_uuid(check_pkl.string_child("Id"));

	cxml::Document check_assetmap("AssetMap");
	check_assetmap.read_file(path / "ASSETMAP.xml");

	BOOST_CHECK_EQUAL(check_assetmap.string_child("Issuer"), "my great issuer");
	BOOST_CHECK_EQUAL(check_assetmap.string_child("Creator"), "the creator");

	BOOST_CHECK(id_in_xml(check_assetmap, pkl_id));

	BOOST_CHECK(id_in_xml(check_assetmap, in_picture->id()));
	BOOST_CHECK(id_in_xml(check_assetmap, in_sound->id()));
	BOOST_CHECK(id_in_xml(check_assetmap, in_cpl->id()));
	BOOST_CHECK(id_in_xml(check_assetmap, add_picture->id()));
	BOOST_CHECK(id_in_xml(check_assetmap, add_sound->id()));
	BOOST_CHECK(id_in_xml(check_assetmap, add_cpl->id()));
}

