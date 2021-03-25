/*
    Copyright (C) 2020 Carl Hetherington <cth@carlh.net>

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


#include "sound_asset.h"
#include "sound_asset_reader.h"
#include "sound_asset_writer.h"
#include "test.h"
#include <boost/filesystem.hpp>
#include <memory>
#include <boost/test/unit_test.hpp>
#include <vector>


using std::vector;
using boost::shared_array;
using std::shared_ptr;


static const int sample_A = 0.038 * 8388608;
static const int sample_B = 0.092 * 8388608;
static const int sample_C = 0.071 * 8388608;

static
bool
close (int reference, int sample)
{
	return abs (reference - sample) < 4096;
}


static
bool
close (int ref1, int ref2, int ref3, int ref4, int check[4])
{
	return close(ref1, check[0]) && close(ref2, check[1]) && close(ref3, check[2]) && close(ref4, check[3]);
}


static int const sync_channel = 13;
static int const bytes_per_sample = 3;


static int
read_sync_sample (uint8_t const* data, int sample_index, int channels)
{
	uint8_t const* p = data + (sample_index * channels * bytes_per_sample) + sync_channel * bytes_per_sample;
	return static_cast<int>((p[0] << 8) | (p[1] << 16) | (p[2] << 24)) / 256;
}


BOOST_AUTO_TEST_CASE (sync_test1)
{
	dcp::SoundAsset asset (private_test / "data" / "atmos_pcm.mxf");
	shared_ptr<dcp::SoundAssetReader> reader = asset.start_read ();
	shared_ptr<const dcp::SoundFrame> frame = reader->get_frame (0);

	/* Read the samples from the first MXF frame of channel 14 and decode them to bits */
	uint8_t const * data = frame->data ();
	vector<bool> ref;
	/* There's 2000 samples which contain 500 bits of data */
	for (int i = 0; i < 500; ++i) {
		int bit[4];
		for (int j = 0; j < 4; ++j) {
			int sample_index = i * 4 + j;
			bit[j] = read_sync_sample (data, sample_index, asset.channels());
		}

		if (close(sample_A, sample_B, sample_B, sample_A, bit)) {
			ref.push_back (false);
		} else if (close(-sample_A, -sample_B, -sample_B, -sample_A, bit)) {
			ref.push_back (false);
		} else if (close(sample_C, sample_C, -sample_C, -sample_C, bit)) {
			ref.push_back (true);
		} else if (close(-sample_C, -sample_C, sample_C, sample_C, bit)) {
			ref.push_back (true);
		} else {
			BOOST_CHECK (false);
		}
	}

	shared_ptr<dcp::SoundAssetWriter> writer = asset.start_write ("build/test/foo.mxf", true);

	/* Compare the sync bits made by SoundAssetWriter to the "proper" ones in the MXF */
	BOOST_CHECK (ref == writer->create_sync_packets());
}


BOOST_AUTO_TEST_CASE (sync_test2)
{
	/* Make a MXF with the same ID as atmos_pcm.mxf and write a frame of random stuff */
	int const channels = 14;
	dcp::SoundAsset asset (dcp::Fraction(24, 1), 48000, channels, dcp::LanguageTag("en-GB"), dcp::Standard::SMPTE);
	asset._id = "e004046e09234f90a4ae4355e7e83506";
	boost::system::error_code ec;
	boost::filesystem::remove ("build/test/foo.mxf", ec);
	auto writer = asset.start_write ("build/test/foo.mxf", true);

	int const frames = 2000;
	float** junk = new float*[channels];
	for (int i = 0; i < channels; ++i) {
		junk[i] = new float[frames];
		for (int j = 0; j < frames; ++j) {
			junk[i][j] = float(rand()) / RAND_MAX;
		}
	}

	writer->write (junk, frames);
	for (int i = 0; i < channels; ++i) {
		delete[] junk[i];
	}
	delete[] junk;
	writer->finalize ();

	/* Check that channel 14 on the first frame matches channel 14 on the reference */
	dcp::SoundAsset ref (private_test / "data" / "atmos_pcm.mxf");
	dcp::SoundAsset check ("build/test/foo.mxf");

	shared_ptr<dcp::SoundAssetReader> ref_read = ref.start_read ();
	shared_ptr<dcp::SoundAssetReader> check_read = check.start_read ();

	shared_ptr<const dcp::SoundFrame> ref_frame = ref_read->get_frame(0);
	uint8_t const* ref_data = ref_frame->data();
	shared_ptr<const dcp::SoundFrame> check_frame = check_read->get_frame(0);
	uint8_t const* check_data = check_frame->data();

	for (int i = 0; i < frames; ++i) {
		int ref_sample = read_sync_sample (ref_data, i, ref.channels());
		int check_sample = read_sync_sample (check_data, i, check.channels());
		BOOST_CHECK (abs(ref_sample - check_sample) < 2);
	}
}

