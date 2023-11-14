/*
    Copyright (C) 2023 Carl Hetherington <cth@carlh.net>

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


#include "mono_mpeg2_picture_asset.h"
#include "mpeg2_transcode.h"
#include "test.h"
#include <boost/test/unit_test.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
}


BOOST_AUTO_TEST_CASE(mpeg_mono_picture_read_test)
{
	dcp::MonoMPEG2PictureAsset asset(private_test / "data" / "mas" / "r2.mxf" );
	std::cout << "frame rate " << asset.frame_rate().numerator << "\n";
	std::cout << "duration " << asset.intrinsic_duration() << "\n";

	auto reader = asset.start_read();

	dcp::MPEG2Decompressor decompressor;
	for (auto i = 0; i < asset.intrinsic_duration(); ++i) {
		auto images = decompressor.decompress_frame(reader->get_frame(i));
		BOOST_CHECK_EQUAL(images.size(), i == 0 ? 0U : 1U);
	}

	auto images = decompressor.flush();
	BOOST_CHECK_EQUAL(images.size(), 1U);
}

