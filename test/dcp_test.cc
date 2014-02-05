/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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

#include "dcp.h"
#include "metadata.h"
#include "cpl.h"
#include "mono_picture_mxf.h"
#include "picture_mxf_writer.h"
#include "sound_mxf_writer.h"
#include "sound_mxf.h"
#include "subtitle_content.h"
#include "reel.h"
#include "test.h"
#include "file.h"
#include "reel_mono_picture_asset.h"
#include "reel_sound_asset.h"
#include "KM_util.h"
#include <sndfile.h>
#include <boost/test/unit_test.hpp>

using boost::shared_ptr;

/* Test creation of a DCP from very simple inputs */
BOOST_AUTO_TEST_CASE (dcp_test)
{
	Kumu::libdcp_test = true;

	/* Some known metadata */
	dcp::XMLMetadata xml_meta;
	xml_meta.issuer = "OpenDCP 0.0.25";
	xml_meta.creator = "OpenDCP 0.0.25";
	xml_meta.issue_date = "2012-07-17T04:45:18+00:00";
	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	/* We're making build/test/foo */
	boost::filesystem::remove_all ("build/test/foo");
	boost::filesystem::create_directories ("build/test/foo");
	dcp::DCP d ("build/test/foo");
	shared_ptr<dcp::CPL> cpl (new dcp::CPL ("A Test DCP", dcp::FEATURE));
	cpl->set_content_version_id ("urn:uri:81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00");
	cpl->set_content_version_label_text ("81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00");

	shared_ptr<dcp::MonoPictureMXF> mp (new dcp::MonoPictureMXF (dcp::Fraction (24, 1)));
	mp->set_metadata (mxf_meta);
	shared_ptr<dcp::PictureMXFWriter> picture_writer = mp->start_write ("build/test/foo/video.mxf", dcp::SMPTE, false);
	dcp::File j2c ("test/data/32x32_red_square.j2c");
	for (int i = 0; i < 24; ++i) {
		picture_writer->write (j2c.data (), j2c.size ());
	}
	picture_writer->finalize ();

	shared_ptr<dcp::SoundMXF> ms (new dcp::SoundMXF (dcp::Fraction (24, 1), 48000, 1));
	ms->set_metadata (mxf_meta);
	shared_ptr<dcp::SoundMXFWriter> sound_writer = ms->start_write ("build/test/foo/audio.mxf", dcp::SMPTE);

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
				  shared_ptr<dcp::ReelSoundAsset> (new dcp::ReelSoundAsset (ms, 0)),
				  shared_ptr<dcp::ReelSubtitleAsset> ()
				  )
			  ));
		  
	d.add (cpl);
	d.add (mp);
	d.add (ms);

	d.write_xml (dcp::SMPTE, xml_meta);

	/* build/test/foo is checked against test/ref/DCP/foo by run-tests.sh */
}
