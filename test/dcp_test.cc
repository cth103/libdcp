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
#include "dcp.h"
#include "metadata.h"
#include "cpl.h"
#include "mono_picture_asset.h"
#include "sound_asset.h"
#include "reel.h"
#include "test.h"
#include "KM_util.h"

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
	shared_ptr<dcp::CPL> cpl (new dcp::CPL ("build/test/foo", "A Test DCP", dcp::FEATURE, 24, 24));

	shared_ptr<dcp::MonoPictureAsset> mp (new dcp::MonoPictureAsset ("build/test/foo", "video.mxf"));
	mp->set_progress (&d.Progress);
	mp->set_edit_rate (24);
	mp->set_intrinsic_duration (24);
	mp->set_duration (24);
	mp->set_size (dcp::Size (32, 32));
	mp->set_metadata (mxf_meta);
	mp->create (j2c);

	shared_ptr<dcp::SoundAsset> ms (new dcp::SoundAsset ("build/test/foo", "audio.mxf"));
	ms->set_progress (&d.Progress);
	ms->set_edit_rate (24);
	ms->set_intrinsic_duration (24);
	ms->set_duration (24);
	ms->set_channels (2);
	ms->set_metadata (mxf_meta);
	ms->create (wav);
	
	cpl->add_reel (shared_ptr<dcp::Reel> (new dcp::Reel (mp, ms, shared_ptr<dcp::SubtitleAsset> ())));
	d.add_cpl (cpl);

	d.write_xml (false, xml_meta);

	/* build/test/foo is checked against test/ref/DCP/foo by run-tests.sh */
}
