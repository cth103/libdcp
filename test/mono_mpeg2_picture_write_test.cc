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


BOOST_AUTO_TEST_CASE(mpeg_mono_picture_write_test)
{
	boost::filesystem::path dir = "build/test/mpeg2_mono_picture_write_test";

	boost::filesystem::remove_all(dir);
	boost::filesystem::create_directories(dir);

	dcp::MonoMPEG2PictureAsset asset(dcp::Fraction{24, 1});
	auto writer = asset.start_write(dir / "test.mxf", dcp::Behaviour::MAKE_NEW);

	dcp::MPEG2Compressor compressor({1920, 1080}, 24, 50000000);
	dcp::FFmpegImage image(int64_t(0));
	for (auto y = 0; y < 1080; ++y) {
		uint8_t* py = image.y() + y * image.y_stride();
		for (auto x = 0; x < 1920; ++x) {
			if (x < 640) {
				*py++ = 76;
			} else if (x < 1280) {
				*py++ = 149;
			} else {
				*py++ = 29;
			}
		}
	}

	for (auto y = 0; y < 540; ++y) {
		uint8_t* pu = image.u() + y * image.u_stride();
		uint8_t* pv = image.v() + y * image.v_stride();
		for (auto x = 0; x < 960; ++x) {
			if (x < 320) {
				*pu++ = 84;
				*pv++ = 255;
			} else if (x < 640) {
				*pu++ = 43;
				*pv++ = 21;
			} else {
				*pu++ = 255;
				*pv++ = 107;
			}
		}
	}

	for (auto i = 0; i < 24; ++i) {
		image.set_pts(int64_t(i));
		if (auto compressed = compressor.compress_frame(image)) {
			writer->write(compressed->first->data(), compressed->first->size());
		}
	}

	if (auto compressed = compressor.flush()) {
		writer->write(compressed->first->data(), compressed->first->size());
	}

	writer->finalize();
}

