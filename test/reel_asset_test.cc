/*
    Copyright (C) 2015 Carl Hetherington <cth@carlh.net>

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

#include "reel_mono_picture_asset.h"
#include <libcxml/cxml.h>
#include <boost/test/unit_test.hpp>

using boost::shared_ptr;

/** Test the XML constructor of ReelPictureAsset */
BOOST_AUTO_TEST_CASE (reel_picture_asset_test)
{
	shared_ptr<cxml::Document> doc (new cxml::Document ("MainPicture"));

	doc->read_string (
		"<MainPicture>"
		"<Id>urn:uuid:06ac1ca7-9c46-4107-8864-a6448e24b04b</Id>"
		"<AnnotationText></AnnotationText>"
		"<EditRate>24 1</EditRate>"
		"<IntrinsicDuration>187048</IntrinsicDuration>"
		"<EntryPoint>0</EntryPoint>"
		"<Duration>187048</Duration>"
		"<Hash>6EQX4NjG8vxIWhLUtHhrGSyLgOY=</Hash>"
		"<FrameRate>24 1</FrameRate>"
		"<ScreenAspectRatio>2048 858</ScreenAspectRatio>"
		"</MainPicture>"
		);

	dcp::ReelMonoPictureAsset pa (doc);
	BOOST_CHECK_EQUAL (pa.screen_aspect_ratio(), dcp::Fraction (2048, 858));
}
