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

#include <boost/test/unit_test.hpp>
#include <libcxml/cxml.h>
#include "cpl.h"
#include "mono_picture_asset.h"

using boost::shared_ptr;

/* Test for a reported bug where <ScreenAspectRatio> in Interop files uses
   excessive decimal places and (sometimes) the wrong decimal point character.
*/
BOOST_AUTO_TEST_CASE (cpl_sar)
{
	shared_ptr<libdcp::MonoPictureAsset> mp (new libdcp::MonoPictureAsset ("test/ref/DCP/foo", "video.mxf"));
	mp->set_interop (true);

	{
		mp->set_size (libdcp::Size (1998, 1080));
		xmlpp::Document doc;
		xmlpp::Element* el = doc.create_root_node ("Test");
		mp->write_to_cpl (el);
		
		cxml::Node node (el);
		BOOST_CHECK_EQUAL (node.node_child("MainPicture")->string_child ("ScreenAspectRatio"), "1.85");
	}

	{
		mp->set_size (libdcp::Size (2048, 858));
		xmlpp::Document doc;
		xmlpp::Element* el = doc.create_root_node ("Test");
		mp->write_to_cpl (el);
		
		cxml::Node node (el);
		BOOST_CHECK_EQUAL (node.node_child("MainPicture")->string_child ("ScreenAspectRatio"), "2.39");
	}
}
