/*
    Copyright (C) 2014-2021 Carl Hetherington <cth@carlh.net>

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

#include "rgb_xyz.h"
#include "openjpeg_image.h"
#include "colour_conversion.h"
#include "stream_operators.h"
#include <boost/test/unit_test.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>

using std::max;
using std::list;
using std::string;
using std::cout;
using std::shared_ptr;
using std::make_shared;
using boost::optional;
using boost::scoped_array;

/** Convert a test image from sRGB to XYZ and check that the transforms are right */
BOOST_AUTO_TEST_CASE (rgb_xyz_test)
{
	srand (0);
	dcp::Size const size (640, 480);

	scoped_array<uint8_t> rgb (new uint8_t[size.width * size.height * 6]);
	for (int y = 0; y < size.height; ++y) {
		uint16_t* p = reinterpret_cast<uint16_t*> (rgb.get() + y * size.width * 6);
		for (int x = 0; x < size.width; ++x) {
			/* Write a 12-bit random number for each component */
			for (int c = 0; c < 3; ++c) {
				*p = (rand () & 0xfff) << 4;
				++p;
			}
		}
	}

	auto xyz = dcp::rgb_to_xyz (rgb.get(), size, size.width * 6, dcp::ColourConversion::srgb_to_xyz());

	for (int y = 0; y < size.height; ++y) {
		uint16_t* p = reinterpret_cast<uint16_t*> (rgb.get() + y * size.width * 6);
		for (int x = 0; x < size.width; ++x) {

			double cr = *p++ / 65535.0;
			double cg = *p++ / 65535.0;
			double cb = *p++ / 65535.0;

			/* Input gamma */

			if (cr < 0.04045) {
				cr /= 12.92;
			} else {
				cr = pow ((cr + 0.055) / 1.055, 2.4);
			}

			if (cg < 0.04045) {
				cg /= 12.92;
			} else {
				cg = pow ((cg + 0.055) / 1.055, 2.4);
			}

			if (cb < 0.04045) {
				cb /= 12.92;
			} else {
				cb = pow ((cb + 0.055) / 1.055, 2.4);
			}

			/* Matrix */

			double cx = cr * 0.4124564 + cg * 0.3575761 + cb * 0.1804375;
			double cy = cr * 0.2126729 + cg * 0.7151522 + cb * 0.0721750;
			double cz = cr * 0.0193339 + cg * 0.1191920 + cb * 0.9503041;

			/* Compand */

			cx *= 48 / 52.37;
			cy *= 48 / 52.37;
			cz *= 48 / 52.37;

			/* Output gamma */

			cx = pow (cx, 1 / 2.6);
			cy = pow (cy, 1 / 2.6);
			cz = pow (cz, 1 / 2.6);

			BOOST_REQUIRE_CLOSE (cx * 4095, xyz->data(0)[y * size.width + x], 1);
			BOOST_REQUIRE_CLOSE (cy * 4095, xyz->data(1)[y * size.width + x], 1);
			BOOST_REQUIRE_CLOSE (cz * 4095, xyz->data(2)[y * size.width + x], 1);
		}
	}
}

static list<string> notes;

static void
note_handler (dcp::NoteType n, string s)
{
	BOOST_REQUIRE_EQUAL (n, dcp::NoteType::NOTE);
	notes.push_back (s);
}

/** Check that xyz_to_rgb clamps XYZ values correctly */
BOOST_AUTO_TEST_CASE (xyz_rgb_range_test)
{
	auto xyz = make_shared<dcp::OpenJPEGImage>(dcp::Size(2, 2));

	xyz->data(0)[0] = -4;
	xyz->data(0)[1] = 6901;
	xyz->data(0)[2] = 0;
	xyz->data(0)[3] = 4095;
	xyz->data(1)[0] = -4;
	xyz->data(1)[1] = 6901;
	xyz->data(1)[2] = 0;
	xyz->data(1)[3] = 4095;
	xyz->data(2)[0] = -4;
	xyz->data(2)[1] = 6901;
	xyz->data(2)[2] = 0;
	xyz->data(2)[3] = 4095;

	scoped_array<uint8_t> rgb (new uint8_t[2 * 2 * 6]);

	notes.clear ();
	dcp::xyz_to_rgb (
		xyz, dcp::ColourConversion::srgb_to_xyz (), rgb.get(), 2 * 6, boost::optional<dcp::NoteHandler> (boost::bind (&note_handler, _1, _2))
		);

	/* The 6 out-of-range samples should have been noted */
	BOOST_REQUIRE_EQUAL (notes.size(), 6);
	auto i = notes.begin ();
	BOOST_REQUIRE_EQUAL (*i++, "XYZ value -4 out of range");
	BOOST_REQUIRE_EQUAL (*i++, "XYZ value -4 out of range");
	BOOST_REQUIRE_EQUAL (*i++, "XYZ value -4 out of range");
	BOOST_REQUIRE_EQUAL (*i++, "XYZ value 6901 out of range");
	BOOST_REQUIRE_EQUAL (*i++, "XYZ value 6901 out of range");
	BOOST_REQUIRE_EQUAL (*i++, "XYZ value 6901 out of range");

	/* And those samples should have been clamped, so check that they give the same result
	   as inputs at the extremes (0 and 4095).
	*/

	auto buffer = reinterpret_cast<uint16_t*> (rgb.get ());
	BOOST_REQUIRE_EQUAL (buffer[0 * 3 + 0], buffer[2 * 3 + 1]);
	BOOST_REQUIRE_EQUAL (buffer[0 * 3 + 1], buffer[2 * 3 + 1]);
	BOOST_REQUIRE_EQUAL (buffer[0 * 3 + 2], buffer[2 * 3 + 2]);

	BOOST_REQUIRE_EQUAL (buffer[1 * 3 + 0], buffer[3 * 3 + 0]);
	BOOST_REQUIRE_EQUAL (buffer[1 * 3 + 1], buffer[3 * 3 + 1]);
	BOOST_REQUIRE_EQUAL (buffer[1 * 3 + 2], buffer[3 * 3 + 2]);
}

/** Convert an image from RGB to XYZ and back again */
BOOST_AUTO_TEST_CASE (rgb_xyz_round_trip_test)
{
	srand (0);
	dcp::Size const size (640, 480);

	scoped_array<uint8_t> rgb (new uint8_t[size.width * size.height * 6]);
	for (int y = 0; y < size.height; ++y) {
		uint16_t* p = reinterpret_cast<uint16_t*> (rgb.get() + y * size.width * 6);
		for (int x = 0; x < size.width; ++x) {
			/* Write a 12-bit random number for each component */
			for (int c = 0; c < 3; ++c) {
				*p = (rand () & 0xfff) << 4;
				++p;
			}
		}
	}

	shared_ptr<dcp::OpenJPEGImage> xyz = dcp::rgb_to_xyz (rgb.get(), size, size.width * 6, dcp::ColourConversion::srgb_to_xyz ());
	scoped_array<uint8_t> back (new uint8_t[size.width * size.height * 6]);
	dcp::xyz_to_rgb (xyz, dcp::ColourConversion::srgb_to_xyz (), back.get(), size.width * 6);

#if 0
	uint16_t* p = reinterpret_cast<uint16_t*> (rgb.get ());
	uint16_t* q = reinterpret_cast<uint16_t*> (back.get ());
	for (int i = 0; i < (size.width * size.height); ++i) {
		/* XXX: doesn't quite work */
		// BOOST_REQUIRE_EQUAL (*p++, *q++);
	}
#endif
}
