/*
    Copyright (C) 2016-2019 Carl Hetherington <cth@carlh.net>

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

#include "mono_picture_asset.h"
#include "mono_picture_asset_writer.h"
#include "j2k_transcode.h"
#include "openjpeg_image.h"
#include <boost/test/unit_test.hpp>

using std::string;
using std::shared_ptr;
using std::make_shared;

static void
check (unsigned int* seed, shared_ptr<dcp::PictureAssetWriter> writer, string hash)
{
	shared_ptr<dcp::OpenJPEGImage> xyz (new dcp::OpenJPEGImage (dcp::Size (1998, 1080)));
	for (int c = 0; c < 3; ++c) {
		for (int p = 0; p < (1998 * 1080); ++p) {
			xyz->data(c)[p] = rand_r (seed) & 0xfff;
		}
	}

	dcp::ArrayData data = dcp::compress_j2k (xyz, 100000000, 24, false, false);

	dcp::FrameInfo info = writer->write (data.data(), data.size());
	BOOST_CHECK_EQUAL (info.hash, hash);
}

/** Test the hashing of data written to JPEG2000 MXFs with some random inputs */
BOOST_AUTO_TEST_CASE (frame_info_hash_test)
{
	auto mp = make_shared<dcp::MonoPictureAsset>(dcp::Fraction (24, 1), dcp::Standard::SMPTE);
	auto writer = mp->start_write ("build/test/frame_info_hash_test.mxf", false);

	unsigned int seed = 42;

	/* Check a few random frames */
	check (&seed, writer, "9da3d1d93a80683e65d996edae4101ed");
	check (&seed, writer, "ecd77b3fbf459591f24119d4118783fb");
	check (&seed, writer, "9f10303495b58ccb715c893d40127e22");
}
