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


#include "sound_asset.h"
#include "sound_asset_writer.h"
#include <boost/filesystem.hpp>
#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <functional>


using std::shared_ptr;


static
void
no_padding_test(boost::filesystem::path path, std::function<void (shared_ptr<dcp::SoundAssetWriter>, boost::random::mt19937&, boost::random::uniform_int_distribution<>&)> write)
{
	dcp::SoundAsset asset({24, 1}, 48000, 6, dcp::LanguageTag{"en-GB"}, dcp::Standard::SMPTE);
	auto writer = asset.start_write(path, {}, dcp::SoundAsset::AtmosSync::DISABLED, dcp::SoundAsset::MCASubDescriptors::ENABLED);

	boost::random::mt19937 rng(1);
	boost::random::uniform_int_distribution<> dist(0, 32767);

	write(writer, rng, dist);
	writer->finalize();

	dcp::SoundAsset check(path);
	auto reader = check.start_read();
	auto frame = reader->get_frame(0);

	rng.seed(1);

	for (auto channel = 0; channel < 6; ++channel) {
		for (auto sample = 0; sample < 2000; ++sample) {
			BOOST_REQUIRE_EQUAL(frame->get(channel, sample), dist(rng));
		}
	}
}


BOOST_AUTO_TEST_CASE(sound_asset_writer_float_no_padding_test)
{
	auto path = boost::filesystem::path("build/test/sound_asset_writer_float_no_padding_test.mxf");

	auto write = [](shared_ptr<dcp::SoundAssetWriter> writer, boost::random::mt19937& rng, boost::random::uniform_int_distribution<>& dist) {
		std::vector<std::vector<float>> buffers(6);
		float* pointers[6];
		for (auto channel = 0; channel < 6; ++channel) {
			buffers[channel].resize(2000);
			for (int sample = 0; sample < 2000; ++sample) {
				buffers[channel][sample] = static_cast<float>(dist(rng)) / (1 << 23);
			}
			pointers[channel] = buffers[channel].data();
		}

		writer->write(pointers, 6, 2000);
	};

	no_padding_test(path, write);
}


BOOST_AUTO_TEST_CASE(sound_asset_writer_int_no_padding_test)
{
	auto path = boost::filesystem::path("build/test/sound_asset_writer_int_no_padding_test.mxf");

	auto write = [](shared_ptr<dcp::SoundAssetWriter> writer, boost::random::mt19937& rng, boost::random::uniform_int_distribution<>& dist) {
		std::vector<std::vector<int32_t>> buffers(6);
		int32_t* pointers[6];
		for (auto channel = 0; channel < 6; ++channel) {
			buffers[channel].resize(2000);
			for (int sample = 0; sample < 2000; ++sample) {
				buffers[channel][sample] = dist(rng);
			}
			pointers[channel] = buffers[channel].data();
		}

		writer->write(pointers, 6, 2000);
	};

	no_padding_test(path, write);
}


static
void
padding_test(boost::filesystem::path path, std::function<void (shared_ptr<dcp::SoundAssetWriter>, boost::random::mt19937&, boost::random::uniform_int_distribution<>&)> write)
{
	dcp::SoundAsset asset({24, 1}, 48000, 14, dcp::LanguageTag{"en-GB"}, dcp::Standard::SMPTE);
	auto writer = asset.start_write(path, {}, dcp::SoundAsset::AtmosSync::DISABLED, dcp::SoundAsset::MCASubDescriptors::ENABLED);

	boost::random::mt19937 rng(1);
	boost::random::uniform_int_distribution<> dist(0, 32767);

	write(writer, rng, dist);
	writer->finalize();

	dcp::SoundAsset check(path);
	auto reader = check.start_read();
	auto frame = reader->get_frame(0);

	rng.seed(1);

	for (auto channel = 0; channel < 6; ++channel) {
		for (auto sample = 0; sample < 2000; ++sample) {
			BOOST_REQUIRE_EQUAL(frame->get(channel, sample), dist(rng));
		}
	}

	for (auto channel = 7; channel < 14; ++channel) {
		for (auto sample = 0; sample < 2000; ++sample) {
			BOOST_REQUIRE_EQUAL(frame->get(channel, sample), 0);
		}
	}
}


BOOST_AUTO_TEST_CASE(sound_asset_writer_float_padding_test)
{
	auto path = boost::filesystem::path("build/test/sound_asset_writer_float_padding_test.mxf");

	auto write = [](shared_ptr<dcp::SoundAssetWriter> writer, boost::random::mt19937& rng, boost::random::uniform_int_distribution<>& dist) {
		std::vector<std::vector<float>> buffers(6);
		float* pointers[6];
		for (auto channel = 0; channel < 6; ++channel) {
			buffers[channel].resize(2000);
			for (int sample = 0; sample < 2000; ++sample) {
				buffers[channel][sample] = static_cast<float>(dist(rng)) / (1 << 23);
			}
			pointers[channel] = buffers[channel].data();
		}

		writer->write(pointers, 6, 2000);
	};

	padding_test(path, write);
}


BOOST_AUTO_TEST_CASE(sound_asset_writer_int_padding_test)
{
	auto path = boost::filesystem::path("build/test/sound_asset_writer_int_padding_test.mxf");

	auto write = [](shared_ptr<dcp::SoundAssetWriter> writer, boost::random::mt19937& rng, boost::random::uniform_int_distribution<>& dist) {
		std::vector<std::vector<int32_t>> buffers(6);
		int32_t* pointers[6];
		for (auto channel = 0; channel < 6; ++channel) {
			buffers[channel].resize(2000);
			for (int sample = 0; sample < 2000; ++sample) {
				buffers[channel][sample] = dist(rng);
			}
			pointers[channel] = buffers[channel].data();
		}

		writer->write(pointers, 6, 2000);
	};

	padding_test(path, write);
}
