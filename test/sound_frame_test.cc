/*
    Copyright (C) 2015-2019 Carl Hetherington <cth@carlh.net>

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

#include <boost/test/unit_test.hpp>
#include "test.h"
#include "sound_frame.h"
#include "sound_asset.h"
#include "sound_asset_reader.h"
#include "exceptions.h"
#include <sndfile.h>

using std::shared_ptr;

BOOST_AUTO_TEST_CASE (sound_frame_test)
{
	int const frame_length = 2000;
	int const channels = 6;

	dcp::SoundAsset asset (
		private_test / "TONEPLATES-SMPTE-PLAINTEXT_TST_F_XX-XX_ITL-TD_51-XX_2K_WOE_20111001_WOE_OV/pcm_95734608-5d47-4d3f-bf5f-9e9186b66afa_.mxf"
		);

	shared_ptr<const dcp::SoundFrame> frame = asset.start_read()->get_frame(42);

	BOOST_REQUIRE_EQUAL (frame->size(), channels * frame_length * 3);

	boost::filesystem::path ref_file = private_test / "data" / "frame.wav";
	SF_INFO info;
	info.format = 0;
	SNDFILE* sndfile = sf_open (ref_file.string().c_str(), SFM_READ, &info);
	BOOST_REQUIRE (sndfile);

	int ref_data[frame_length * channels];
	int const read = sf_readf_int (sndfile, ref_data, frame_length);
	BOOST_REQUIRE_EQUAL (read, frame_length);

	/* Check raw data is as we expect */
	uint8_t const * p = frame->data ();
	for (int i = 0; i < (frame_length * channels); ++i) {
		int x = ref_data[i] >> 8;
		if (x < 0) {
			x = (1 << 24) + x;
		}
		int const y = p[0] | (p[1] << 8) | (p[2] << 16);
		BOOST_REQUIRE_EQUAL (x, y);
		p += 3;
	}

	/* Check SoundFrame::get() */
	int* ref = ref_data;
	for (int sample = 0; sample < frame_length; ++sample) {
		for (int channel = 0; channel < channels; ++channel) {
			BOOST_REQUIRE_EQUAL ((*ref++) >> 8, frame->get(channel, sample));
		}
	}
}

BOOST_AUTO_TEST_CASE (sound_frame_test2)
{
	{
		dcp::ASDCPErrorSuspender sus;
		BOOST_CHECK_THROW (dcp::SoundAsset("frobozz"), dcp::FileError);
	}

	dcp::SoundAsset asset (
		private_test /
		"TONEPLATES-SMPTE-PLAINTEXT_TST_F_XX-XX_ITL-TD_51-XX_2K_WOE_20111001_WOE_OV/pcm_95734608-5d47-4d3f-bf5f-9e9186b66afa_.mxf"
		);

	BOOST_CHECK_THROW (asset.start_read()->get_frame (99999999), dcp::ReadError);
}
