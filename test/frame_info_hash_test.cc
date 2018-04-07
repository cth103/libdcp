/*
    Copyright (C) 2016 Carl Hetherington <cth@carlh.net>

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

#include "mono_picture_asset.h"
#include "mono_picture_asset_writer.h"
#include "j2k.h"
#include "openjpeg_image.h"
#include <boost/test/unit_test.hpp>

using std::string;
using boost::shared_ptr;

static void
check (unsigned int* seed, shared_ptr<dcp::PictureAssetWriter> writer, string hash)
{
	shared_ptr<dcp::OpenJPEGImage> xyz (new dcp::OpenJPEGImage (dcp::Size (1998, 1080)));
	for (int c = 0; c < 3; ++c) {
		for (int p = 0; p < (1998 * 1080); ++p) {
			xyz->data(c)[p] = rand_r (seed) & 0xfff;
		}
	}

	dcp::Data data = dcp::compress_j2k (xyz, 100000000, 24, false, false);

	dcp::FrameInfo info = writer->write (data.data().get(), data.size());
	BOOST_CHECK_EQUAL (info.hash, hash);
}

/** Test the hashing of data written to JPEG2000 MXFs with some random inputs */
BOOST_AUTO_TEST_CASE (frame_info_hash_test)
{
	shared_ptr<dcp::MonoPictureAsset> mp (new dcp::MonoPictureAsset (dcp::Fraction (24, 1), dcp::SMPTE));
	shared_ptr<dcp::PictureAssetWriter> writer = mp->start_write ("build/test/frame_info_hash_test.mxf", false);

	unsigned int seed = 42;

	/* Check a few random frames */
	check (&seed, writer, "bb19bc96466d1b65708dabafa2e2ec0f");
	check (&seed, writer, "426ec5ac52d7a27fe278f7736d53151f");
	check (&seed, writer, "fafb05a0039cb9fc604279c90a13cb87");
}
