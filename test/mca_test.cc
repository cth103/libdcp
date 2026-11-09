/*
    Copyright (C) 2020-2021 Carl Hetherington <cth@carlh.net>

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
#include <asdcp/Metadata.h>
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
		cpl.set_main_sound_configuration(dcp::MainSoundConfiguration("51/L,R,C,LFE,Ls,Rs"));
		cpl.set_main_sound_sample_rate(48000);
		cpl.set_main_picture_stored_area(dcp::Size(1998, 1080));
		cpl.set_main_picture_active_area(dcp::Size(1998, 1080));
		cpl.write_xml (dcp::String::compose("build/test/parse_mca_descriptors_from_mxf_test%1/cpl.xml", i), shared_ptr<dcp::CertificateChain>());

		cxml::Document ref("CompositionPlaylist", private_test / dcp::String::compose("51_sound_with_mca_%1.cpl", i));
		cxml::Document check("CompositionPlaylist", dcp::String::compose("build/test/parse_mca_descriptors_from_mxf_test%1/cpl.xml", i));

		vector<string> ignore;
		check_xml (
			dynamic_cast<xmlpp::Element const*>(
				ref.node_child("ReelList")->node_children("Reel").front()->node_child("AssetList")->node_child("CompositionMetadataAsset")->node_child("MCASubDescriptors")->node()
				),
			dynamic_cast<xmlpp::Element const*>(
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
	auto writer = sound_asset->start_write("build/test/write_mca_descriptors_to_mxf_test.mxf", {}, dcp::SoundAsset::AtmosSync::DISABLED, dcp::SoundAsset::MCASubDescriptors::ENABLED);

	float* samples[6];
	for (int i = 0; i < 6; ++i) {
		samples[i] = new float[2000];
		memset (samples[i], 0, 2000 * sizeof(float));
	}
	for (int i = 0; i < 24; ++i) {
		writer->write(samples, 6, 2000);
	}
	for (int i = 0; i < 6; ++i) {
		delete[] samples[i];
	}

	writer->finalize();

	/* Make a CPL as a roundabout way to read the metadata we just wrote to the MXF */

	auto reel_sound_asset = make_shared<dcp::ReelSoundAsset>(sound_asset, 0);
	auto reel = make_shared<dcp::Reel>();
	reel->add (black_picture_asset("build/test/write_mca_descriptors_to_mxf_test", 24));
	reel->add (reel_sound_asset);

	dcp::CPL cpl("", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	cpl.add (reel);
	cpl.set_main_sound_configuration(dcp::MainSoundConfiguration("51/L,R,C,LFE,Ls,Rs"));
	cpl.set_main_sound_sample_rate(48000);
	cpl.set_main_picture_stored_area(dcp::Size(1998, 1080));
	cpl.set_main_picture_active_area(dcp::Size(1998, 1080));
	cpl.write_xml ("build/test/write_mca_descriptors_to_mxf_test/cpl.xml", shared_ptr<dcp::CertificateChain>());

	cxml::Document ref("CompositionPlaylist", private_test / "51_sound_with_mca_1.cpl");
	cxml::Document check("CompositionPlaylist", "build/test/write_mca_descriptors_to_mxf_test/cpl.xml");

	check_xml (
		dynamic_cast<xmlpp::Element const*>(
			ref.node_child("ReelList")->node_children("Reel")[0]->node_child("AssetList")->node_child("CompositionMetadataAsset")->node_child("MCASubDescriptors")->node()
			),
		dynamic_cast<xmlpp::Element const*>(
			check.node_child("ReelList")->node_children("Reel")[0]->node_child("AssetList")->node_child("CompositionMetadataAsset")->node_child("MCASubDescriptors")->node()
			),
		{ "InstanceID", "MCALinkID", "SoundfieldGroupLinkID" },
		true
		);
}


static
void
check_mca_descriptors(int suffix, vector<dcp::Channel> extra_active_channels, vector<string> expected_mca_tag_symbols)
{
	auto const dir = boost::filesystem::path(dcp::String::compose("build/test/check_mca_descriptors_%1", suffix));
	boost::filesystem::remove_all(dir);
	boost::filesystem::create_directories(dir);

	auto sound_asset = make_shared<dcp::SoundAsset>(dcp::Fraction(24, 1), 48000, 16, dcp::LanguageTag("en-US"), dcp::Standard::SMPTE);
	auto writer = sound_asset->start_write(dir / "mxf.mxf", extra_active_channels, dcp::SoundAsset::AtmosSync::DISABLED, dcp::SoundAsset::MCASubDescriptors::ENABLED);

	vector<vector<float>> samples(6);
	float* pointers[6];
	for (int i = 0; i < 6; ++i) {
		samples[i].resize(2000);
		pointers[i] = samples[i].data();
	}
	for (int i = 0; i < 24; ++i) {
		writer->write(pointers, 6, 2000);
	}
	writer->finalize();

	/* Check MXF */

	Kumu::FileReaderFactory factory;
	auto reader = new ASDCP::PCM::MXFReader(factory);
	reader->OpenRead(boost::filesystem::path(dir / "mxf.mxf").string());

	list<ASDCP::MXF::InterchangeObject*> channels;
	auto const r = reader->OP1aHeader().GetMDObjectsByType(
		dcp::asdcp_smpte_dict->ul(ASDCP::MDD_AudioChannelLabelSubDescriptor),
		channels
		);
	BOOST_REQUIRE(KM_SUCCESS(r));

	vector<string> mxf_mca_tag_symbols;
	for (auto channel: channels) {
		auto audio_channel = reinterpret_cast<ASDCP::MXF::AudioChannelLabelSubDescriptor*>(channel);
		char buffer[64];
		audio_channel->MCATagSymbol.EncodeString(buffer, sizeof(buffer));
		mxf_mca_tag_symbols.push_back(buffer);
	}

	BOOST_CHECK(expected_mca_tag_symbols == mxf_mca_tag_symbols);

	/* Check CPL */

	auto reel_sound_asset = make_shared<dcp::ReelSoundAsset>(sound_asset, 0);
	auto reel = make_shared<dcp::Reel>();
	reel->add(black_picture_asset(dir / "dcp", 24));
	reel->add(reel_sound_asset);

	dcp::CPL cpl("", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	cpl.add(reel);
	cpl.set_main_sound_configuration(dcp::MainSoundConfiguration("51/L,R,C,LFE,Ls,Rs"));
	cpl.set_main_sound_sample_rate(48000);
	cpl.set_main_picture_stored_area(dcp::Size(1998, 1080));
	cpl.set_main_picture_active_area(dcp::Size(1998, 1080));
	cpl.write_xml(dir / "dcp" / "cpl.xml", shared_ptr<dcp::CertificateChain>());

	cxml::Document check("CompositionPlaylist", dir / "dcp" / "cpl.xml");
	vector<string> cpl_mca_tag_symbols;

	for (auto node: check.node_child("ReelList")->node_child("Reel")->node_child("AssetList")->node_child("CompositionMetadataAsset")->node_child("MCASubDescriptors")->node_children("AudioChannelLabelSubDescriptor")) {
		cpl_mca_tag_symbols.push_back(node->string_child("MCATagSymbol"));
	}

	BOOST_CHECK(expected_mca_tag_symbols == cpl_mca_tag_symbols);
}


BOOST_AUTO_TEST_CASE(write_correct_mca_descriptors)
{
	int suffix = 0;

	check_mca_descriptors(
		suffix++,
		{}, { "chL", "chR", "chC", "chLFE", "chLs", "chRs" }
		);

	check_mca_descriptors(
		suffix++,
		{ dcp::Channel::HI }, { "chL", "chR", "chC", "chLFE", "chLs", "chRs", "chHI" }
		);

	check_mca_descriptors(
		suffix++,
		{ dcp::Channel::VI }, { "chL", "chR", "chC", "chLFE", "chLs", "chRs", "chVIN" }
		);

	check_mca_descriptors(
		suffix++,
		{ dcp::Channel::BSL }, { "chL", "chR", "chC", "chLFE", "chLss", "chRss", "chLrs" }
		);

	check_mca_descriptors(
		suffix++,
		{ dcp::Channel::BSR }, { "chL", "chR", "chC", "chLFE", "chLss", "chRss", "chRrs" }
		);

	check_mca_descriptors(
		suffix++,
		{ dcp::Channel::HI, dcp::Channel::VI }, { "chL", "chR", "chC", "chLFE", "chLs", "chRs", "chHI", "chVIN" }
		);

	check_mca_descriptors(
		suffix++,
		{ dcp::Channel::HI, dcp::Channel::VI, dcp::Channel::BSL, dcp::Channel::BSR }, { "chL", "chR", "chC", "chLFE", "chLss", "chRss", "chHI", "chVIN", "chLrs", "chRrs" }
		);

	check_mca_descriptors(
		suffix++,
		{ dcp::Channel::BSL, dcp::Channel::BSR }, { "chL", "chR", "chC", "chLFE", "chLss", "chRss", "chLrs", "chRrs" }
		);

	/* Duplicates should be ignored */
	check_mca_descriptors(
		suffix++,
		{ dcp::Channel::HI, dcp::Channel::HI }, { "chL", "chR", "chC", "chLFE", "chLs", "chRs", "chHI" }
		);

	/* Sign language video gets its own label, whose symbol has no "ch" (SMPTE RDD 52 table 4) */
	check_mca_descriptors(
		suffix++,
		{ dcp::Channel::SIGN_LANGUAGE }, { "chL", "chR", "chC", "chLFE", "chLs", "chRs", "SLVS" }
		);

	check_mca_descriptors(
		suffix++,
		{ dcp::Channel::HI, dcp::Channel::VI, dcp::Channel::SIGN_LANGUAGE }, { "chL", "chR", "chC", "chLFE", "chLs", "chRs", "chHI", "chVIN", "SLVS" }
		);
}


/** The sign language video channel label has the values of SMPTE RDD 52 table 4 (from ISDCF Doc 13) */
BOOST_AUTO_TEST_CASE(write_sign_language_video_mca_descriptor)
{
	auto const dir = boost::filesystem::path("build/test/write_sign_language_video_mca_descriptor");
	boost::filesystem::remove_all(dir);
	boost::filesystem::create_directories(dir);

	auto sound_asset = make_shared<dcp::SoundAsset>(dcp::Fraction(24, 1), 48000, 16, dcp::LanguageTag("ca"), dcp::Standard::SMPTE);
	auto writer = sound_asset->start_write(dir / "mxf.mxf", { dcp::Channel::SIGN_LANGUAGE }, dcp::SoundAsset::AtmosSync::DISABLED, dcp::SoundAsset::MCASubDescriptors::ENABLED);

	vector<float> samples(2000);
	float* pointers[1] = { samples.data() };
	for (int i = 0; i < 24; ++i) {
		writer->write(pointers, 1, 2000);
	}
	writer->finalize();

	Kumu::FileReaderFactory factory;
	ASDCP::PCM::MXFReader reader(factory);
	BOOST_REQUIRE(KM_SUCCESS(reader.OpenRead(boost::filesystem::path(dir / "mxf.mxf").string())));

	list<ASDCP::MXF::InterchangeObject*> channels;
	BOOST_REQUIRE(KM_SUCCESS(reader.OP1aHeader().GetMDObjectsByType(dcp::asdcp_smpte_dict->ul(ASDCP::MDD_AudioChannelLabelSubDescriptor), channels)));

	static byte_t const sign_language_ul[] = {
		0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x0d, 0x0f, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00
	};

	int found = 0;
	for (auto object: channels) {
		auto channel = reinterpret_cast<ASDCP::MXF::AudioChannelLabelSubDescriptor*>(object);
		char buffer[64];
		channel->MCATagSymbol.EncodeString(buffer, sizeof(buffer));
		if (string(buffer) != "SLVS") {
			continue;
		}
		++found;
		channel->MCATagName.get().EncodeString(buffer, sizeof(buffer));
		BOOST_CHECK_EQUAL(string(buffer), "Sign Language Video Stream");
		BOOST_REQUIRE(!channel->MCAChannelID.empty());
		BOOST_CHECK_EQUAL(channel->MCAChannelID.get(), 15U);
		BOOST_CHECK(channel->MCALabelDictionaryID == ASDCP::UL(sign_language_ul));
		BOOST_REQUIRE(!channel->RFC5646SpokenLanguage.empty());
		channel->RFC5646SpokenLanguage.get().EncodeString(buffer, sizeof(buffer));
		BOOST_CHECK_EQUAL(string(buffer), "ca");
	}
	BOOST_CHECK_EQUAL(found, 1);

	/* And libdcp reads it back as a labelled channel */
	dcp::SoundAsset check(dir / "mxf.mxf");
	BOOST_CHECK(check.labelled_channels().count(static_cast<int>(dcp::Channel::SIGN_LANGUAGE)) == 1);
}

