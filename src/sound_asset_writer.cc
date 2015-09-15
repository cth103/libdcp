/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "sound_asset_writer.h"
#include "sound_asset.h"
#include "exceptions.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include "AS_DCP.h"

using namespace dcp;

struct SoundAssetWriter::ASDCPState
{
	ASDCP::PCM::MXFWriter mxf_writer;
	ASDCP::PCM::FrameBuffer frame_buffer;
	ASDCP::WriterInfo writer_info;
	ASDCP::PCM::AudioDescriptor audio_desc;
};

SoundAssetWriter::SoundAssetWriter (SoundAsset* asset, boost::filesystem::path file, Standard standard)
	: AssetWriter (asset, file)
	, _state (new SoundAssetWriter::ASDCPState)
	, _sound_asset (asset)
	, _frame_buffer_offset (0)
{
	/* Derived from ASDCP::Wav::SimpleWaveHeader::FillADesc */
	_state->audio_desc.EditRate = ASDCP::Rational (_sound_asset->edit_rate().numerator, _sound_asset->edit_rate().denominator);
	_state->audio_desc.AudioSamplingRate = ASDCP::Rational (_sound_asset->sampling_rate(), 1);
	_state->audio_desc.Locked = 0;
	_state->audio_desc.ChannelCount = _sound_asset->channels ();
	_state->audio_desc.QuantizationBits = 24;
	_state->audio_desc.BlockAlign = 3 * _sound_asset->channels();
	_state->audio_desc.AvgBps = _sound_asset->sampling_rate() * _state->audio_desc.BlockAlign;
	_state->audio_desc.LinkedTrackID = 0;
	_state->audio_desc.ChannelFormat = ASDCP::PCM::CF_NONE;

	_state->frame_buffer.Capacity (ASDCP::PCM::CalcFrameBufferSize (_state->audio_desc));
	_state->frame_buffer.Size (ASDCP::PCM::CalcFrameBufferSize (_state->audio_desc));
	memset (_state->frame_buffer.Data(), 0, _state->frame_buffer.Capacity());

	_sound_asset->fill_writer_info (&_state->writer_info, _sound_asset->id(), standard);
}

void
SoundAssetWriter::write (float const * const * data, int frames)
{
	DCP_ASSERT (!_finalized);

	if (!_started) {
		Kumu::Result_t r = _state->mxf_writer.OpenWrite (_file.string().c_str(), _state->writer_info, _state->audio_desc);
		if (ASDCP_FAILURE (r)) {
			boost::throw_exception (FileError ("could not open audio MXF for writing", _file.string(), r));
		}

		_sound_asset->set_file (_file);
		_started = true;
	}

	for (int i = 0; i < frames; ++i) {

		byte_t* out = _state->frame_buffer.Data() + _frame_buffer_offset;

		/* Write one sample per channel */
		for (int j = 0; j < _sound_asset->channels(); ++j) {
			int32_t const s = data[j][i] * (1 << 23);
			*out++ = (s & 0xff);
			*out++ = (s & 0xff00) >> 8;
			*out++ = (s & 0xff0000) >> 16;
		}
		_frame_buffer_offset += 3 * _sound_asset->channels();

		DCP_ASSERT (_frame_buffer_offset <= int (_state->frame_buffer.Capacity()));

		/* Finish the MXF frame if required */
		if (_frame_buffer_offset == int (_state->frame_buffer.Capacity())) {
			write_current_frame ();
			_frame_buffer_offset = 0;
			memset (_state->frame_buffer.Data(), 0, _state->frame_buffer.Capacity());
		}
	}
}

void
SoundAssetWriter::write_current_frame ()
{
	ASDCP::Result_t const r = _state->mxf_writer.WriteFrame (_state->frame_buffer, _encryption_context, 0);
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MiscError (String::compose ("could not write audio MXF frame (%1)", int (r))));
	}

	++_frames_written;
}

bool
SoundAssetWriter::finalize ()
{
	if (_frame_buffer_offset > 0) {
		write_current_frame ();
	}

	if (_started && ASDCP_FAILURE (_state->mxf_writer.Finalize())) {
		boost::throw_exception (MiscError ("could not finalise audio MXF"));
	}

	_sound_asset->_intrinsic_duration = _frames_written;
	return AssetWriter::finalize ();
}
