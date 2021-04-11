/*
    Copyright (C) 2013-2020 Carl Hetherington <cth@carlh.net>

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
#include "metadata.h"
#include "cpl.h"
#include "mono_picture_asset.h"
#include "stereo_picture_asset.h"
#include "picture_asset_writer.h"
#include "reel_picture_asset.h"
#include "sound_asset_writer.h"
#include "sound_asset.h"
#include "atmos_asset.h"
#include "reel.h"
#include "test.h"
#include "reel_mono_picture_asset.h"
#include "reel_stereo_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_atmos_asset.h"
#include "reel_markers_asset.h"
#include <asdcp/KM_util.h>
#include <sndfile.h>
#include <boost/test/unit_test.hpp>

using std::string;
using std::vector;
using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::make_shared;
#if BOOST_VERSION >= 106100
using namespace boost::placeholders;
#endif


/** Test creation of a 2D SMPTE DCP from very simple inputs */
BOOST_AUTO_TEST_CASE (dcp_test1)
{
	RNGFixer fixer;

	make_simple("build/test/DCP/dcp_test1")->write_xml(
		"OpenDCP 0.0.25", "OpenDCP 0.0.25", "2012-07-17T04:45:18+00:00", "A Test DCP"
		);

	/* build/test/DCP/dcp_test1 is checked against test/ref/DCP/dcp_test1 by run/tests */
}

/** Test creation of a 3D DCP from very simple inputs */
BOOST_AUTO_TEST_CASE (dcp_test2)
{
	RNGFixer fix;

	/* Some known metadata */
	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	/* We're making build/test/DCP/dcp_test2 */
	boost::filesystem::remove_all ("build/test/DCP/dcp_test2");
	boost::filesystem::create_directories ("build/test/DCP/dcp_test2");
	dcp::DCP d ("build/test/DCP/dcp_test2");
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	cpl->set_content_version (
		dcp::ContentVersion("urn:uri:81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00", "81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00")
		);
	cpl->set_issuer ("OpenDCP 0.0.25");
	cpl->set_creator ("OpenDCP 0.0.25");
	cpl->set_issue_date ("2012-07-17T04:45:18+00:00");
	cpl->set_annotation_text ("A Test DCP");

	auto mp = make_shared<dcp::StereoPictureAsset>(dcp::Fraction (24, 1), dcp::Standard::SMPTE);
	mp->set_metadata (mxf_meta);
	auto picture_writer = mp->start_write ("build/test/DCP/dcp_test2/video.mxf", false);
	dcp::ArrayData j2c ("test/data/flat_red.j2c");
	for (int i = 0; i < 24; ++i) {
		/* Left */
		picture_writer->write (j2c.data (), j2c.size ());
		/* Right */
		picture_writer->write (j2c.data (), j2c.size ());
	}
	picture_writer->finalize ();

	auto ms = make_shared<dcp::SoundAsset>(dcp::Fraction(24, 1), 48000, 1, dcp::LanguageTag("en-GB"), dcp::Standard::SMPTE);
	ms->set_metadata (mxf_meta);
	auto sound_writer = ms->start_write ("build/test/DCP/dcp_test2/audio.mxf");

	SF_INFO info;
	info.format = 0;
	auto sndfile = sf_open ("test/data/1s_24-bit_48k_silence.wav", SFM_READ, &info);
	BOOST_CHECK (sndfile);
	float buffer[4096*6];
	float* channels[1];
	channels[0] = buffer;
	while (true) {
		auto N = sf_readf_float (sndfile, buffer, 4096);
		sound_writer->write (channels, N);
		if (N < 4096) {
			break;
		}
	}

	sound_writer->finalize ();

	cpl->add (make_shared<dcp::Reel>(
			  make_shared<dcp::ReelStereoPictureAsset>(mp, 0),
			  make_shared<dcp::ReelSoundAsset>(ms, 0)
			  ));

	d.add (cpl);

	d.write_xml ("OpenDCP 0.0.25", "OpenDCP 0.0.25", "2012-07-17T04:45:18+00:00", "Created by libdcp");

	/* build/test/DCP/dcp_test2 is checked against test/ref/DCP/dcp_test2 by run/tests */
}

static void
note (dcp::NoteType, string)
{

}

/** Test comparison of a DCP with itself */
BOOST_AUTO_TEST_CASE (dcp_test3)
{
	dcp::DCP A ("test/ref/DCP/dcp_test1");
	A.read ();
	dcp::DCP B ("test/ref/DCP/dcp_test1");
	B.read ();

	BOOST_CHECK (A.equals (B, dcp::EqualityOptions(), boost::bind (&note, _1, _2)));
}

/** Test comparison of a DCP with a different DCP */
BOOST_AUTO_TEST_CASE (dcp_test4)
{
	dcp::DCP A ("test/ref/DCP/dcp_test1");
	A.read ();
	dcp::DCP B ("test/ref/DCP/dcp_test2");
	B.read ();

	BOOST_CHECK (!A.equals (B, dcp::EqualityOptions(), boost::bind (&note, _1, _2)));
}

static
void
test_rewriting_sound(string name, bool modify)
{
	using namespace boost::filesystem;

	dcp::DCP A ("test/ref/DCP/dcp_test1");
	A.read ();

	BOOST_REQUIRE (!A.cpls().empty());
	BOOST_REQUIRE (!A.cpls().front()->reels().empty());
	auto A_picture = dynamic_pointer_cast<dcp::ReelMonoPictureAsset>(A.cpls().front()->reels().front()->main_picture());
	BOOST_REQUIRE (A_picture);
	auto A_sound = dynamic_pointer_cast<dcp::ReelSoundAsset>(A.cpls().front()->reels().front()->main_sound());

	string const picture = "j2c_5279f9aa-94d7-42a6-b0e0-e4eaec4e2a15.mxf";

	remove_all ("build/test/" + name);
	dcp::DCP B ("build/test/" + name);
	auto reel = make_shared<dcp::Reel>();

	BOOST_REQUIRE (A_picture->mono_asset());
	BOOST_REQUIRE (A_picture->mono_asset()->file());
	copy_file (A_picture->mono_asset()->file().get(), path("build") / "test" / name / picture);
	reel->add(make_shared<dcp::ReelMonoPictureAsset>(make_shared<dcp::MonoPictureAsset>(path("build") / "test" / name / picture), 0));

	auto reader = A_sound->asset()->start_read();
	auto sound = make_shared<dcp::SoundAsset>(A_sound->asset()->edit_rate(), A_sound->asset()->sampling_rate(), A_sound->asset()->channels(), dcp::LanguageTag("en-US"), dcp::Standard::SMPTE);
	auto writer = sound->start_write(path("build") / "test" / name / "pcm_8246f87f-e1df-4c42-a290-f3b3069ff021.mxf", {});

	bool need_to_modify = modify;
	for (int i = 0; i < A_sound->asset()->intrinsic_duration(); ++i) {
		auto sf = reader->get_frame (i);
		float* out[sf->channels()];
		for (int j = 0; j < sf->channels(); ++j) {
			out[j] = new float[sf->samples()];
		}
		for (int j = 0; j < sf->samples(); ++j) {
			for (int k = 0; k < sf->channels(); ++k) {
				out[k][j] = static_cast<float>(sf->get(k, j)) / (1 << 23);
				if (need_to_modify) {
					out[k][j] += 1.0 / (1 << 23);
					need_to_modify = false;
				}
			}
		}
		writer->write (out, sf->samples());
		for (int j = 0; j < sf->channels(); ++j) {
			delete[] out[j];
		}
	}
	writer->finalize();

	reel->add(make_shared<dcp::ReelSoundAsset>(sound, 0));
	reel->add(simple_markers());

	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->add (reel);

	B.add (cpl);
	B.write_xml ();

	dcp::EqualityOptions eq;
	eq.reel_hashes_can_differ = true;
	eq.max_audio_sample_error = 0;
	if (modify) {
		BOOST_CHECK (!A.equals(B, eq, boost::bind(&note, _1, _2)));
	} else {
		BOOST_CHECK (A.equals(B, eq, boost::bind(&note, _1, _2)));
	}
}

/** Test comparison of a DCP with another that has the same picture and the same (but re-written) sound */
BOOST_AUTO_TEST_CASE (dcp_test9)
{
	test_rewriting_sound ("dcp_test9", false);
}

/** Test comparison of a DCP with another that has the same picture and very slightly modified sound */
BOOST_AUTO_TEST_CASE (dcp_test10)
{
	test_rewriting_sound ("dcp_test10", true);
}

/** Test creation of a 2D DCP with an Atmos track */
BOOST_AUTO_TEST_CASE (dcp_test5)
{
	RNGFixer fix;

	/* Some known metadata */
	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	/* We're making build/test/DCP/dcp_test5 */
	boost::filesystem::remove_all ("build/test/DCP/dcp_test5");
	boost::filesystem::create_directories ("build/test/DCP/dcp_test5");
	dcp::DCP d ("build/test/DCP/dcp_test5");
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	cpl->set_content_version (
		dcp::ContentVersion("urn:uri:81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00", "81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00")
		);
	cpl->set_issuer ("OpenDCP 0.0.25");
	cpl->set_creator ("OpenDCP 0.0.25");
	cpl->set_issue_date ("2012-07-17T04:45:18+00:00");
	cpl->set_annotation_text ("A Test DCP");

	auto mp = make_shared<dcp::MonoPictureAsset>(dcp::Fraction (24, 1), dcp::Standard::SMPTE);
	mp->set_metadata (mxf_meta);
	shared_ptr<dcp::PictureAssetWriter> picture_writer = mp->start_write ("build/test/DCP/dcp_test5/video.mxf", false);
	dcp::ArrayData j2c ("test/data/flat_red.j2c");
	for (int i = 0; i < 24; ++i) {
		picture_writer->write (j2c.data (), j2c.size ());
	}
	picture_writer->finalize ();

	auto ms = make_shared<dcp::SoundAsset>(dcp::Fraction(24, 1), 48000, 1, dcp::LanguageTag("en-GB"), dcp::Standard::SMPTE);
	ms->set_metadata (mxf_meta);
	auto sound_writer = ms->start_write ("build/test/DCP/dcp_test5/audio.mxf");

	SF_INFO info;
	info.format = 0;
	SNDFILE* sndfile = sf_open ("test/data/1s_24-bit_48k_silence.wav", SFM_READ, &info);
	BOOST_CHECK (sndfile);
	float buffer[4096*6];
	float* channels[1];
	channels[0] = buffer;
	while (true) {
		sf_count_t N = sf_readf_float (sndfile, buffer, 4096);
		sound_writer->write (channels, N);
		if (N < 4096) {
			break;
		}
	}

	sound_writer->finalize ();

	shared_ptr<dcp::AtmosAsset> am (new dcp::AtmosAsset (private_test / "20160218_NameOfFilm_FTR_OV_EN_A_dcs_r01.mxf"));

	cpl->add (shared_ptr<dcp::Reel> (
			  new dcp::Reel (
				  shared_ptr<dcp::ReelMonoPictureAsset> (new dcp::ReelMonoPictureAsset (mp, 0)),
				  shared_ptr<dcp::ReelSoundAsset> (new dcp::ReelSoundAsset (ms, 0)),
				  shared_ptr<dcp::ReelSubtitleAsset> (),
				  shared_ptr<dcp::ReelMarkersAsset> (),
				  shared_ptr<dcp::ReelAtmosAsset> (new dcp::ReelAtmosAsset (am, 0))
				  )
			  ));

	d.add (cpl);

	d.write_xml ("OpenDCP 0.0.25", "OpenDCP 0.0.25", "2012-07-17T04:45:18+00:00", "Created by libdcp");

	/* build/test/DCP/dcp_test5 is checked against test/ref/DCP/dcp_test5 by run/tests */
}

/** Basic tests of reading a 2D DCP with an Atmos track */
BOOST_AUTO_TEST_CASE (dcp_test6)
{
	dcp::DCP dcp ("test/ref/DCP/dcp_test5");
	dcp.read ();

	BOOST_REQUIRE_EQUAL (dcp.cpls().size(), 1);
	BOOST_REQUIRE_EQUAL (dcp.cpls().front()->reels().size(), 1);
	BOOST_CHECK (dcp.cpls().front()->reels().front()->main_picture());
	BOOST_CHECK (dcp.cpls().front()->reels().front()->main_sound());
	BOOST_CHECK (!dcp.cpls().front()->reels().front()->main_subtitle());
	BOOST_CHECK (dcp.cpls().front()->reels().front()->atmos());
}

/** Test creation of a 2D Interop DCP from very simple inputs */
BOOST_AUTO_TEST_CASE (dcp_test7)
{
	RNGFixer fix;

	make_simple("build/test/DCP/dcp_test7", 1, 24, dcp::Standard::INTEROP)->write_xml(
		"OpenDCP 0.0.25", "OpenDCP 0.0.25", "2012-07-17T04:45:18+00:00", "Created by libdcp"
		);

	/* build/test/DCP/dcp_test7 is checked against test/ref/DCP/dcp_test7 by run/tests */
}

/** Test reading of a DCP with multiple CPLs */
BOOST_AUTO_TEST_CASE (dcp_test8)
{
	dcp::DCP dcp (private_test / "data/SMPTE_TST-B1PB2P_S_EN-EN-CCAP_5171-HI-VI_2K_ISDCF_20151123_DPPT_SMPTE_combo/");
	dcp.read ();

	BOOST_REQUIRE_EQUAL (dcp.cpls().size(), 2);
}


/** Test reading a DCP whose ASSETMAP contains assets not used by any PKL */
BOOST_AUTO_TEST_CASE (dcp_things_in_assetmap_not_in_pkl)
{
	dcp::DCP dcp ("test/data/extra_assetmap");
	BOOST_CHECK_NO_THROW (dcp.read());
}


/** Test that writing the XML for a DCP with no CPLs throws */
BOOST_AUTO_TEST_CASE (dcp_with_no_cpls)
{
	dcp::DCP dcp ("build/test/dcp_with_no_cpls");
	BOOST_REQUIRE_THROW (dcp.write_xml(), dcp::MiscError);
}


/** Test that writing the XML for a DCP with Interop CPLs makes a SMPTE assetmap */
BOOST_AUTO_TEST_CASE (dcp_with_interop_cpls)
{
	boost::filesystem::path path = "build/test/dcp_with_interop_cpls";
	boost::filesystem::remove_all (path);
	dcp::DCP dcp (path);
	auto cpl1 = make_shared<dcp::CPL>("", dcp::ContentKind::FEATURE, dcp::Standard::INTEROP);
	cpl1->add(make_shared<dcp::Reel>());
	dcp.add(cpl1);
	auto cpl2 = make_shared<dcp::CPL>("", dcp::ContentKind::FEATURE, dcp::Standard::INTEROP);
	cpl2->add(make_shared<dcp::Reel>());
	dcp.add(cpl2);
	dcp.write_xml ();
	BOOST_REQUIRE (boost::filesystem::exists(path / "ASSETMAP"));
	BOOST_REQUIRE (!boost::filesystem::exists(path / "ASSETMAP.xml"));
}


/** Test that writing the XML for a DCP with SMPTE CPLs makes a SMPTE assetmap */
BOOST_AUTO_TEST_CASE (dcp_with_smpte_cpls)
{
	boost::filesystem::path path = "build/test/dcp_with_smpte_cpls";
	boost::filesystem::remove_all (path);
	dcp::DCP dcp (path);
	auto cpl1 = make_shared<dcp::CPL>("", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	cpl1->add(make_shared<dcp::Reel>());
	dcp.add(cpl1);
	auto cpl2 = make_shared<dcp::CPL>("", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	cpl2->add(make_shared<dcp::Reel>());
	dcp.add(cpl2);
	dcp.write_xml ();
	BOOST_REQUIRE (!boost::filesystem::exists(path / "ASSETMAP"));
	BOOST_REQUIRE (boost::filesystem::exists(path / "ASSETMAP.xml"));
}


/** Test that writing the XML for a DCP with mixed-standard CPLs throws */
BOOST_AUTO_TEST_CASE (dcp_with_mixed_cpls)
{
	dcp::DCP dcp ("build/test/dcp_with_mixed_cpls");
	dcp.add(make_shared<dcp::CPL>("", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE));
	dcp.add(make_shared<dcp::CPL>("", dcp::ContentKind::FEATURE, dcp::Standard::INTEROP));
	dcp.add(make_shared<dcp::CPL>("", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE));
	BOOST_REQUIRE_THROW (dcp.write_xml(), dcp::MiscError);
}
