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
#include "picture_asset.h"
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
	libdcp::XMLMetadata xml_meta;
	xml_meta.issuer = "OpenDCP 0.0.25";
	xml_meta.creator = "OpenDCP 0.0.25";
	xml_meta.issue_date = "2012-07-17T04:45:18+00:00";
	libdcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	/* We're making build/test/foo */
	boost::filesystem::remove_all ("build/test/foo");
	boost::filesystem::create_directories ("build/test/foo");
	libdcp::DCP d ("build/test/foo");
	shared_ptr<libdcp::CPL> cpl (new libdcp::CPL ("build/test/foo", "A Test DCP", libdcp::FEATURE, 24, 24));

	shared_ptr<libdcp::MonoPictureAsset> mp (new libdcp::MonoPictureAsset (
							 j2c,
							 "build/test/foo",
							 "video.mxf",
							 &d.Progress,
							 24,
							 24,
							 libdcp::Size (32, 32),
							 false,
							 mxf_meta
							 ));

	shared_ptr<libdcp::SoundAsset> ms (new libdcp::SoundAsset (
						   wav,
						   "build/test/foo",
						   "audio.mxf",
						   &(d.Progress),
						   24,
						   24,
						   2,
						   false,
						   mxf_meta
						   ));
	
	cpl->add_reel (shared_ptr<libdcp::Reel> (new libdcp::Reel (mp, ms, shared_ptr<libdcp::SubtitleAsset> ())));
	d.add_cpl (cpl);

	d.write_xml (false, xml_meta);

	/* build/test/foo is checked against test/ref/DCP/foo by run-tests.sh */
}
