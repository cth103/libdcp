/*
    Copyright (C) 2013-2017 Carl Hetherington <cth@carlh.net>

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
*/

#include "dcp.h"
#include "metadata.h"
#include "cpl.h"
#include "mono_picture_asset.h"
#include "stereo_picture_asset.h"
#include "picture_asset_writer.h"
#include "sound_asset_writer.h"
#include "sound_asset.h"
#include "atmos_asset.h"
#include "reel.h"
#include "test.h"
#include "file.h"
#include "reel_mono_picture_asset.h"
#include "reel_stereo_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_atmos_asset.h"
#include <asdcp/KM_util.h>
#include <sndfile.h>
#include <boost/test/unit_test.hpp>

using std::string;
using boost::shared_ptr;

static shared_ptr<dcp::DCP>
make_simple (boost::filesystem::path path)
{
	Kumu::cth_test = true;

	/* Some known metadata */
	dcp::XMLMetadata xml_meta;
	xml_meta.annotation_text = "A Test DCP";
	xml_meta.issuer = "OpenDCP 0.0.25";
	xml_meta.creator = "OpenDCP 0.0.25";
	xml_meta.issue_date = "2012-07-17T04:45:18+00:00";
	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	/* We're making build/test/DCP/dcp_test1 */
	boost::filesystem::remove_all (path);
	boost::filesystem::create_directories (path);
	shared_ptr<dcp::DCP> d (new dcp::DCP (path));
	shared_ptr<dcp::CPL> cpl (new dcp::CPL ("A Test DCP", dcp::FEATURE));
	cpl->set_content_version_id ("urn:uri:81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00");
	cpl->set_content_version_label_text ("81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00");
	cpl->set_metadata (xml_meta);

	shared_ptr<dcp::MonoPictureAsset> mp (new dcp::MonoPictureAsset (dcp::Fraction (24, 1), dcp::SMPTE));
	mp->set_metadata (mxf_meta);
	shared_ptr<dcp::PictureAssetWriter> picture_writer = mp->start_write (path / "video.mxf", false);
	dcp::File j2c ("test/data/32x32_red_square.j2c");
	for (int i = 0; i < 24; ++i) {
		picture_writer->write (j2c.data (), j2c.size ());
	}
	picture_writer->finalize ();

	shared_ptr<dcp::SoundAsset> ms (new dcp::SoundAsset (dcp::Fraction (24, 1), 48000, 1, dcp::SMPTE));
	ms->set_metadata (mxf_meta);
	shared_ptr<dcp::SoundAssetWriter> sound_writer = ms->start_write (path / "audio.mxf");

	SF_INFO info;
	info.format = 0;
	SNDFILE* sndfile = sf_open ("test/data/1s_24-bit_48k_silence.wav", SFM_READ, &info);
	BOOST_CHECK (sndfile);
	float buffer[4096*6];
	float* channels[1];
	channels[0] = buffer;
	while (1) {
		sf_count_t N = sf_readf_float (sndfile, buffer, 4096);
		sound_writer->write (channels, N);
		if (N < 4096) {
			break;
		}
	}

	sound_writer->finalize ();

	cpl->add (shared_ptr<dcp::Reel> (
			  new dcp::Reel (
				  shared_ptr<dcp::ReelMonoPictureAsset> (new dcp::ReelMonoPictureAsset (mp, 0)),
				  shared_ptr<dcp::ReelSoundAsset> (new dcp::ReelSoundAsset (ms, 0))
				  )
			  ));

	d->add (cpl);
	return d;
}

/** Test creation of a 2D SMPTE DCP from very simple inputs */
BOOST_AUTO_TEST_CASE (dcp_test1)
{
	dcp::XMLMetadata xml_meta;
	xml_meta.annotation_text = "Created by libdcp";
	xml_meta.issuer = "OpenDCP 0.0.25";
	xml_meta.creator = "OpenDCP 0.0.25";
	xml_meta.issue_date = "2012-07-17T04:45:18+00:00";
	make_simple("build/test/DCP/dcp_test1")->write_xml (dcp::SMPTE, xml_meta);
	/* build/test/DCP/dcp_test1 is checked against test/ref/DCP/dcp_test1 by run/tests */
}

/** Test creation of a 3D DCP from very simple inputs */
BOOST_AUTO_TEST_CASE (dcp_test2)
{
	Kumu::cth_test = true;

	/* Some known metadata */
	dcp::XMLMetadata xml_meta;
	xml_meta.annotation_text = "A Test DCP";
	xml_meta.issuer = "OpenDCP 0.0.25";
	xml_meta.creator = "OpenDCP 0.0.25";
	xml_meta.issue_date = "2012-07-17T04:45:18+00:00";
	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	/* We're making build/test/DCP/dcp_test2 */
	boost::filesystem::remove_all ("build/test/DCP/dcp_test2");
	boost::filesystem::create_directories ("build/test/DCP/dcp_test2");
	dcp::DCP d ("build/test/DCP/dcp_test2");
	shared_ptr<dcp::CPL> cpl (new dcp::CPL ("A Test DCP", dcp::FEATURE));
	cpl->set_content_version_id ("urn:uri:81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00");
	cpl->set_content_version_label_text ("81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00");
	cpl->set_metadata (xml_meta);

	shared_ptr<dcp::StereoPictureAsset> mp (new dcp::StereoPictureAsset (dcp::Fraction (24, 1), dcp::SMPTE));
	mp->set_metadata (mxf_meta);
	shared_ptr<dcp::PictureAssetWriter> picture_writer = mp->start_write ("build/test/DCP/dcp_test2/video.mxf", false);
	dcp::File j2c ("test/data/32x32_red_square.j2c");
	for (int i = 0; i < 24; ++i) {
		/* Left */
		picture_writer->write (j2c.data (), j2c.size ());
		/* Right */
		picture_writer->write (j2c.data (), j2c.size ());
	}
	picture_writer->finalize ();

	shared_ptr<dcp::SoundAsset> ms (new dcp::SoundAsset (dcp::Fraction (24, 1), 48000, 1, dcp::SMPTE));
	ms->set_metadata (mxf_meta);
	shared_ptr<dcp::SoundAssetWriter> sound_writer = ms->start_write ("build/test/DCP/dcp_test2/audio.mxf");

	SF_INFO info;
	info.format = 0;
	SNDFILE* sndfile = sf_open ("test/data/1s_24-bit_48k_silence.wav", SFM_READ, &info);
	BOOST_CHECK (sndfile);
	float buffer[4096*6];
	float* channels[1];
	channels[0] = buffer;
	while (1) {
		sf_count_t N = sf_readf_float (sndfile, buffer, 4096);
		sound_writer->write (channels, N);
		if (N < 4096) {
			break;
		}
	}

	sound_writer->finalize ();

	cpl->add (shared_ptr<dcp::Reel> (
			  new dcp::Reel (
				  shared_ptr<dcp::ReelStereoPictureAsset> (new dcp::ReelStereoPictureAsset (mp, 0)),
				  shared_ptr<dcp::ReelSoundAsset> (new dcp::ReelSoundAsset (ms, 0))
				  )
			  ));

	d.add (cpl);

	xml_meta.annotation_text = "Created by libdcp";
	d.write_xml (dcp::SMPTE, xml_meta);

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

/** Test creation of a 2D DCP with an Atmos track */
BOOST_AUTO_TEST_CASE (dcp_test5)
{
	Kumu::cth_test = true;

	/* Some known metadata */
	dcp::XMLMetadata xml_meta;
	xml_meta.annotation_text = "A Test DCP";
	xml_meta.issuer = "OpenDCP 0.0.25";
	xml_meta.creator = "OpenDCP 0.0.25";
	xml_meta.issue_date = "2012-07-17T04:45:18+00:00";
	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	/* We're making build/test/DCP/dcp_test5 */
	boost::filesystem::remove_all ("build/test/DCP/dcp_test5");
	boost::filesystem::create_directories ("build/test/DCP/dcp_test5");
	dcp::DCP d ("build/test/DCP/dcp_test5");
	shared_ptr<dcp::CPL> cpl (new dcp::CPL ("A Test DCP", dcp::FEATURE));
	cpl->set_content_version_id ("urn:uri:81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00");
	cpl->set_content_version_label_text ("81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00");
	cpl->set_metadata (xml_meta);

	shared_ptr<dcp::MonoPictureAsset> mp (new dcp::MonoPictureAsset (dcp::Fraction (24, 1), dcp::SMPTE));
	mp->set_metadata (mxf_meta);
	shared_ptr<dcp::PictureAssetWriter> picture_writer = mp->start_write ("build/test/DCP/dcp_test5/video.mxf", false);
	dcp::File j2c ("test/data/32x32_red_square.j2c");
	for (int i = 0; i < 24; ++i) {
		picture_writer->write (j2c.data (), j2c.size ());
	}
	picture_writer->finalize ();

	shared_ptr<dcp::SoundAsset> ms (new dcp::SoundAsset (dcp::Fraction (24, 1), 48000, 1, dcp::SMPTE));
	ms->set_metadata (mxf_meta);
	shared_ptr<dcp::SoundAssetWriter> sound_writer = ms->start_write ("build/test/DCP/dcp_test5/audio.mxf");

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
				  shared_ptr<dcp::ReelAtmosAsset> (new dcp::ReelAtmosAsset (am, 0))
				  )
			  ));

	d.add (cpl);

	xml_meta.annotation_text = "Created by libdcp";
	d.write_xml (dcp::SMPTE, xml_meta);

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
	dcp::XMLMetadata xml_meta;
	xml_meta.annotation_text = "Created by libdcp";
	xml_meta.issuer = "OpenDCP 0.0.25";
	xml_meta.creator = "OpenDCP 0.0.25";
	xml_meta.issue_date = "2012-07-17T04:45:18+00:00";
	make_simple("build/test/DCP/dcp_test7")->write_xml (dcp::INTEROP, xml_meta);
	/* build/test/DCP/dcp_test7 is checked against test/ref/DCP/dcp_test7 by run/tests */
}

/** Test reading of a DCP with multiple PKLs */
BOOST_AUTO_TEST_CASE (dcp_test8)
{
	dcp::DCP dcp (private_test / "data/SMPTE_TST-B1PB2P_S_EN-EN-CCAP_5171-HI-VI_2K_ISDCF_20151123_DPPT_SMPTE_combo/");
	dcp.read ();

	BOOST_REQUIRE_EQUAL (dcp.cpls().size(), 2);
}
