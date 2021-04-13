/*
    Copyright (C) 2020 Carl Hetherington <cth@carlh.net>

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


#include "compose.hpp"
#include "cpl.h"
#include "reel.h"
#include "reel_sound_asset.h"
#include "sound_asset.h"
#include "sound_asset_writer.h"
#include "test.h"
#include "warnings.h"
#include <libcxml/cxml.h>
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <boost/test/unit_test.hpp>


using std::list;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::vector;


/** Check that when we read a MXF and write its MCA metadata to a CPL we get the same answer
 *  as the original MXF for that CPL (for a couple of different MXFs).
 */
BOOST_AUTO_TEST_CASE (parse_mca_descriptors_from_mxf_test)
{
	for (int i = 1; i < 3; ++i) {
		auto sound_asset = make_shared<dcp::SoundAsset>(private_test / "data" / dcp::String::compose("51_sound_with_mca_%1.mxf", i));
		auto reel_sound_asset = make_shared<dcp::ReelSoundAsset>(sound_asset, 0);
		auto reel = make_shared<dcp::Reel>();
		reel->add (black_picture_asset(dcp::String::compose("build/test/parse_mca_descriptors_from_mxf_test%1", i), 24));
		reel->add (reel_sound_asset);

		dcp::CPL cpl("", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
		cpl.add (reel);
		cpl.set_main_sound_configuration("51/L,R,C,LFE,Ls,Rs");
		cpl.set_main_sound_sample_rate(48000);
		cpl.set_main_picture_stored_area(dcp::Size(1998, 1080));
		cpl.set_main_picture_active_area(dcp::Size(1998, 1080));
		cpl.write_xml (dcp::String::compose("build/test/parse_mca_descriptors_from_mxf_test%1/cpl.xml", i), shared_ptr<dcp::CertificateChain>());

		cxml::Document ref("CompositionPlaylist", private_test / dcp::String::compose("51_sound_with_mca_%1.cpl", i));
		cxml::Document check("CompositionPlaylist", dcp::String::compose("build/test/parse_mca_descriptors_from_mxf_test%1/cpl.xml", i));

		vector<string> ignore;
		check_xml (
			dynamic_cast<xmlpp::Element*>(
				ref.node_child("ReelList")->node_children("Reel").front()->node_child("AssetList")->node_child("CompositionMetadataAsset")->node_child("MCASubDescriptors")->node()
				),
			dynamic_cast<xmlpp::Element*>(
				check.node_child("ReelList")->node_children("Reel").front()->node_child("AssetList")->node_child("CompositionMetadataAsset")->node_child("MCASubDescriptors")->node()
				),
			ignore,
			true
			);
	}
}


/** Reproduce the MCA tags from one of the example files using libdcp */
BOOST_AUTO_TEST_CASE (write_mca_descriptors_to_mxf_test)
{
	auto sound_asset = make_shared<dcp::SoundAsset>(dcp::Fraction(24, 1), 48000, 6, dcp::LanguageTag("en-US"), dcp::Standard::SMPTE);
	auto writer = sound_asset->start_write("build/test/write_mca_descriptors_to_mxf_test.mxf");

	float* samples[6];
	for (int i = 0; i < 6; ++i) {
		samples[i] = new float[2000];
		memset (samples[i], 0, 2000 * sizeof(float));
	}
	for (int i = 0; i < 24; ++i) {
		writer->write(samples, 2000);
	}
	for (int i = 0; i < 6; ++i) {
		delete[] samples[i];
	}

	writer->finalize();

	/* Make a CPL as a roundabout way to read the metadata we just wrote to the MXF */

	shared_ptr<dcp::ReelSoundAsset> reel_sound_asset(new dcp::ReelSoundAsset(sound_asset, 0));
	shared_ptr<dcp::Reel> reel(new dcp::Reel());
	reel->add (black_picture_asset("build/test/write_mca_descriptors_to_mxf_test", 24));
	reel->add (reel_sound_asset);

	dcp::CPL cpl("", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	cpl.add (reel);
	cpl.set_main_sound_configuration("51/L,R,C,LFE,Ls,Rs");
	cpl.set_main_sound_sample_rate(48000);
	cpl.set_main_picture_stored_area(dcp::Size(1998, 1080));
	cpl.set_main_picture_active_area(dcp::Size(1998, 1080));
	cpl.write_xml ("build/test/write_mca_descriptors_to_mxf_test/cpl.xml", shared_ptr<dcp::CertificateChain>());

	cxml::Document ref("CompositionPlaylist", private_test / "51_sound_with_mca_1.cpl");
	cxml::Document check("CompositionPlaylist", "build/test/write_mca_descriptors_to_mxf_test/cpl.xml");

	check_xml (
		dynamic_cast<xmlpp::Element*>(
			ref.node_child("ReelList")->node_children("Reel")[0]->node_child("AssetList")->node_child("CompositionMetadataAsset")->node_child("MCASubDescriptors")->node()
			),
		dynamic_cast<xmlpp::Element*>(
			check.node_child("ReelList")->node_children("Reel")[0]->node_child("AssetList")->node_child("CompositionMetadataAsset")->node_child("MCASubDescriptors")->node()
			),
		{ "InstanceID", "MCALinkID", "SoundfieldGroupLinkID" },
		true
		);
}

