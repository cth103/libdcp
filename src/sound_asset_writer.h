/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/sound_asset_writer.h
 *  @brief SoundAssetWriter class
 */


#include "asset_writer.h"
#include "dcp_assert.h"
#include "fsk.h"
#include "sound_asset.h"
#include "sound_frame.h"
#include <memory>
#include <boost/filesystem.hpp>
#include <boost/shared_array.hpp>


struct sync_test1;


namespace dcp {


namespace sound_asset_writer {

template <typename T>
int32_t convert(T) { return {}; }

template <>
inline int32_t convert(int32_t x)
{
	int constexpr clip = (1 << 23);
	return std::max(-clip, std::min(clip, x));
}

template<>
inline int32_t convert(float x)
{
	float constexpr clip = 1.0f - (1.0f / (1 << 23));
	float constexpr scale = (1 << 23);
	auto const clipped = std::max(-clip, std::min(clip, x));
	return std::lround(clipped * scale);
}

}


class SoundAsset;


/** @class SoundAssetWriter
 *  @brief A helper class for writing to SoundAssets.
 *
 *  Objects of this class can only be created with SoundAsset::start_write().
 *
 *  Sound samples can be written to the SoundAsset by calling write() with
 *  a buffer of float values.  finalize() must be called after the last samples
 *  have been written.
 */
class SoundAssetWriter : public AssetWriter
{
public:
	~SoundAssetWriter();

	/** @param data Pointer an array of float pointers, one for each channel.
	 *  @param channels Number of channels in data; if this is less than the channels in the asset
	 *  the remaining asset channels will be padded with silence.
	 *  @param frames Number of frames i.e. number of floats that are given for each channel.
	 */
	void write(float const * const * data, int channels, int frames);

	/** @param data Pointer an array of int32_t pointers, one for each channel.
	 *  The 24-bit audio sample should be in the lower 24 bits of the int32_t.
	 *  @param channels Number of channels in data; if this is less than the channels in the asset
	 *  the remaining asset channels will be padded with silence.
	 *  @param frames Number of frames i.e. number of floats that are given for each channel.
	 */
	void write(int32_t const * const * data, int channels, int frames);

	bool finalize () override;

private:
	friend class SoundAsset;
	friend struct ::sync_test1;

	byte_t* frame_buffer_data() const;
	int frame_buffer_capacity() const;

	template <class T>
	void
	do_write(T const * const * data, int data_channels, int frames)
	{
		DCP_ASSERT(!_finalized);
		DCP_ASSERT(frames > 0);

		auto const asset_channels = _asset->channels();
		DCP_ASSERT(data_channels <= asset_channels);

		if (!_started) {
			start();
		}

		for (int i = 0; i < frames; ++i) {

			auto out = frame_buffer_data() + _frame_buffer_offset;

			/* Write one sample per asset channel */
			for (int j = 0; j < asset_channels; ++j) {
				int32_t s = 0;
				if (j == 13 && _sync) {
					s = _fsk.get();
				} else if (j < data_channels) {
					s = sound_asset_writer::convert(data[j][i]);
				}
				*out++ = (s & 0xff);
				*out++ = (s & 0xff00) >> 8;
				*out++ = (s & 0xff0000) >> 16;
			}
			_frame_buffer_offset += 3 * asset_channels;

			DCP_ASSERT(_frame_buffer_offset <= frame_buffer_capacity());

			/* Finish the MXF frame if required */
			if (_frame_buffer_offset == frame_buffer_capacity()) {
				write_current_frame();
				_frame_buffer_offset = 0;
				memset(frame_buffer_data(), 0, frame_buffer_capacity());
			}
		}
	}

	SoundAssetWriter(SoundAsset *, boost::filesystem::path, std::vector<dcp::Channel> extra_active_channels, bool sync, bool include_mca_subdescriptors);

	void start ();
	void write_current_frame ();
	std::vector<bool> create_sync_packets ();

	/* do this with an opaque pointer so we don't have to include
	   ASDCP headers
	*/

	struct ASDCPState;
	std::shared_ptr<ASDCPState> _state;

	SoundAsset* _asset = nullptr;
	int _frame_buffer_offset = 0;

	std::vector<dcp::Channel> _extra_active_channels;
	/** true to ignore any signal passed to write() on channel 14 and instead write a sync track */
	bool _sync = false;
	/** index of the sync packet (0-3) which starts the next edit unit */
	int _sync_packet = 0;
	FSK _fsk;
	bool _include_mca_subdescriptors = true;
};

}
