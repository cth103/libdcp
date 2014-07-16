/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#include "cpl.h"
#include "reel_mono_picture_asset.h"
#include "mono_picture_mxf.h"
#include <libcxml/cxml.h>
#include <boost/test/unit_test.hpp>

using boost::shared_ptr;

/* Test for a reported bug where <ScreenAspectRatio> in Interop files uses
   excessive decimal places and (sometimes) the wrong decimal point character.
*/
BOOST_AUTO_TEST_CASE (cpl_sar)
{
	shared_ptr<dcp::ReelMonoPictureAsset> pa (
		new dcp::ReelMonoPictureAsset (
			shared_ptr<dcp::MonoPictureMXF> (new dcp::MonoPictureMXF ("test/ref/DCP/foo/video.mxf")),
			0
			)
		);

	{
		pa->set_screen_aspect_ratio (dcp::Fraction (1998, 1080));
		xmlpp::Document doc;
		xmlpp::Element* el = doc.create_root_node ("Test");
		pa->write_to_cpl (el, dcp::INTEROP);
		
		cxml::Node node (el);
		BOOST_CHECK_EQUAL (node.node_child("MainPicture")->string_child ("ScreenAspectRatio"), "1.85");
	}

	{
		pa->set_screen_aspect_ratio (dcp::Fraction (2048, 858));
		xmlpp::Document doc;
		xmlpp::Element* el = doc.create_root_node ("Test");
		pa->write_to_cpl (el, dcp::INTEROP);
		
		cxml::Node node (el);
		BOOST_CHECK_EQUAL (node.node_child("MainPicture")->string_child ("ScreenAspectRatio"), "2.39");
	}
}
