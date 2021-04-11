/*
    Copyright (C) 2015-2021 Carl Hetherington <cth@carlh.net>

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


#include "reel_mono_picture_asset.h"
#include "reel_smpte_subtitle_asset.h"
#include <libcxml/cxml.h>
#include <boost/test/unit_test.hpp>
#include "stream_operators.h"
#include "test.h"


using std::make_shared;
using std::string;
using std::shared_ptr;
using boost::optional;


/** Test the XML constructor of ReelPictureAsset */
BOOST_AUTO_TEST_CASE (reel_picture_asset_test)
{
	auto doc = make_shared<cxml::Document>("MainPicture");

	doc->read_string (
		"<MainPicture>"
		"<Id>urn:uuid:06ac1ca7-9c46-4107-8864-a6448e24b04b</Id>"
		"<AnnotationText>Hello world!</AnnotationText>"
		"<EditRate>24 1</EditRate>"
		"<IntrinsicDuration>187048</IntrinsicDuration>"
		"<EntryPoint>42</EntryPoint>"
		"<Duration>9444</Duration>"
		"<Hash>6EQX4NjG8vxIWhLUtHhrGSyLgOY=</Hash>"
		"<FrameRate>24 1</FrameRate>"
		"<ScreenAspectRatio>2048 858</ScreenAspectRatio>"
		"</MainPicture>"
		);

	dcp::ReelMonoPictureAsset pa (doc);
	BOOST_CHECK_EQUAL (pa.id(), "06ac1ca7-9c46-4107-8864-a6448e24b04b");
	BOOST_CHECK_EQUAL (pa.annotation_text(), "Hello world!");
	BOOST_CHECK_EQUAL (pa.edit_rate(), dcp::Fraction(24, 1));
	BOOST_CHECK_EQUAL (pa.intrinsic_duration(), 187048);
	BOOST_CHECK_EQUAL (pa.entry_point().get(), 42L);
	BOOST_CHECK_EQUAL (pa.duration().get(), 9444L);
	BOOST_CHECK_EQUAL (pa.hash().get(), string("6EQX4NjG8vxIWhLUtHhrGSyLgOY="));
	BOOST_CHECK_EQUAL (pa.frame_rate(), dcp::Fraction(24, 1));
	BOOST_CHECK_EQUAL (pa.screen_aspect_ratio(), dcp::Fraction(2048, 858));
}


/** Test the XML constructor of ReelSMPTESubtitleAsset */
BOOST_AUTO_TEST_CASE (reel_smpte_subtitle_asset_test)
{
	auto doc = make_shared<cxml::Document>("MainSubtitle");

	doc->read_string (
		"<MainSubtitle>"
		"<Id>urn:uuid:8bca1489-aab1-9259-a4fd-8150abc1de12</Id>"
		"<AnnotationText>Goodbye world!</AnnotationText>"
		"<EditRate>25 1</EditRate>"
		"<IntrinsicDuration>1870</IntrinsicDuration>"
		"<EntryPoint>0</EntryPoint>"
		"<Duration>525</Duration>"
		"<KeyId>urn:uuid:540cbf10-ab14-0233-ab1f-fb31501cabfa</KeyId>"
		"<Hash>3EABjX9BB1CAWhLUtHhrGSyLgOY=</Hash>"
		"<Language>de-DE</Language>"
		"</MainSubtitle>"
		);

	dcp::ReelSMPTESubtitleAsset ps (doc);
	BOOST_CHECK_EQUAL (ps.id(), "8bca1489-aab1-9259-a4fd-8150abc1de12");
	BOOST_CHECK_EQUAL (ps.annotation_text(), "Goodbye world!");
	BOOST_CHECK_EQUAL (ps.edit_rate(), dcp::Fraction(25, 1));
	BOOST_CHECK_EQUAL (ps.intrinsic_duration(), 1870);
	BOOST_CHECK_EQUAL (ps.entry_point().get(), 0L);
	BOOST_CHECK_EQUAL (ps.duration().get(), 525L);
	BOOST_CHECK_EQUAL (ps.hash().get(), string("3EABjX9BB1CAWhLUtHhrGSyLgOY="));
	BOOST_REQUIRE (ps.language());
	BOOST_CHECK_EQUAL (ps.language().get(), "de-DE");
}
