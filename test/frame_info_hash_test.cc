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


#include "j2k_transcode.h"
#include "mono_picture_asset.h"
#include "mono_picture_asset_writer.h"
#include "openjpeg_image.h"
#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>


using std::make_shared;
using std::shared_ptr;
using std::string;


static void
check (shared_ptr<dcp::PictureAssetWriter> writer, boost::random::uniform_int_distribution<>& dist, boost::random::mt19937& rng, string hash)
{
	auto xyz = make_shared<dcp::OpenJPEGImage>(dcp::Size(1998, 1080));
	for (int c = 0; c < 3; ++c) {
		for (int p = 0; p < (1998 * 1080); ++p) {
			xyz->data(c)[p] = dist(rng);
		}
	}

	auto data = dcp::compress_j2k (xyz, 100000000, 24, false, false);

	auto info = writer->write (data.data(), data.size());
	BOOST_CHECK_EQUAL (info.hash, hash);
}


/** Test the hashing of data written to JPEG2000 MXFs with some random inputs */
BOOST_AUTO_TEST_CASE (frame_info_hash_test)
{
	auto mp = make_shared<dcp::MonoPictureAsset>(dcp::Fraction (24, 1), dcp::Standard::SMPTE);
	auto writer = mp->start_write("build/test/frame_info_hash_test.mxf", dcp::PictureAsset::Behaviour::MAKE_NEW);

	boost::random::mt19937 rng(1);
	boost::random::uniform_int_distribution<> dist(0, 4095);

	/* Check a few random frames */
	check(writer, dist, rng, "a9e772602a2fd3135d940cfd727ab8ff");
	check(writer, dist, rng, "b075369922e42b23e1852a586ec43224");
	check(writer, dist, rng, "402395e76152db05b03c8f24ddfd7732");
}
