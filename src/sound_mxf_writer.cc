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

#include "sound_mxf_writer.h"
#include "sound_mxf.h"
#include "exceptions.h"
#include "compose.hpp"
#include "AS_DCP.h"
#include <boost/lexical_cast.hpp>

using boost::lexical_cast;
using namespace dcp;

struct SoundMXFWriter::ASDCPState
{
	ASDCP::PCM::MXFWriter mxf_writer;
	ASDCP::PCM::FrameBuffer frame_buffer;
	ASDCP::WriterInfo writer_info;
	ASDCP::PCM::AudioDescriptor audio_desc;
	ASDCP::AESEncContext* encryption_context;
};

SoundMXFWriter::SoundMXFWriter (SoundMXF* m, boost::filesystem::path file, Standard standard)
	: MXFWriter (m, file)
	, _state (new SoundMXFWriter::ASDCPState)
	, _sound_mxf (m)
	, _frame_buffer_offset (0)
{
	_state->encryption_context = m->encryption_context ();
	
	/* Derived from ASDCP::Wav::SimpleWaveHeader::FillADesc */
	_state->audio_desc.EditRate = ASDCP::Rational (_sound_mxf->edit_rate().numerator, _sound_mxf->edit_rate().denominator);
	_state->audio_desc.AudioSamplingRate = ASDCP::Rational (_sound_mxf->sampling_rate(), 1);
	_state->audio_desc.Locked = 0;
	_state->audio_desc.ChannelCount = _sound_mxf->channels ();
	_state->audio_desc.QuantizationBits = 24;
	_state->audio_desc.BlockAlign = 3 * _sound_mxf->channels();
	_state->audio_desc.AvgBps = _sound_mxf->sampling_rate() * _state->audio_desc.BlockAlign;
	_state->audio_desc.LinkedTrackID = 0;
	_state->audio_desc.ChannelFormat = ASDCP::PCM::CF_NONE;
	
	_state->frame_buffer.Capacity (ASDCP::PCM::CalcFrameBufferSize (_state->audio_desc));
	_state->frame_buffer.Size (ASDCP::PCM::CalcFrameBufferSize (_state->audio_desc));
	memset (_state->frame_buffer.Data(), 0, _state->frame_buffer.Capacity());
	
	_sound_mxf->fill_writer_info (&_state->writer_info, standard);
	
	Kumu::Result_t r = _state->mxf_writer.OpenWrite (file.string().c_str(), _state->writer_info, _state->audio_desc);
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (FileError ("could not open audio MXF for writing", file.string(), r));
	}
}

void
SoundMXFWriter::write (float const * const * data, int frames)
{
	assert (!_finalized);
	
	for (int i = 0; i < frames; ++i) {

		byte_t* out = _state->frame_buffer.Data() + _frame_buffer_offset;

		/* Write one sample per channel */
		for (int j = 0; j < _sound_mxf->channels(); ++j) {
			int32_t const s = data[j][i] * (1 << 23);
			*out++ = (s & 0xff);
			*out++ = (s & 0xff00) >> 8;
			*out++ = (s & 0xff0000) >> 16;
		}
		_frame_buffer_offset += 3 * _sound_mxf->channels();

		assert (_frame_buffer_offset <= int (_state->frame_buffer.Capacity()));

		/* Finish the MXF frame if required */
		if (_frame_buffer_offset == int (_state->frame_buffer.Capacity())) {
			write_current_frame ();
			_frame_buffer_offset = 0;
			memset (_state->frame_buffer.Data(), 0, _state->frame_buffer.Capacity());
		}
	}
}

void
SoundMXFWriter::write_current_frame ()
{
	ASDCP::Result_t const r = _state->mxf_writer.WriteFrame (_state->frame_buffer, _state->encryption_context, 0);
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MiscError (String::compose ("could not write audio MXF frame (%1)", int (r))));
	}

	++_frames_written;
}

void
SoundMXFWriter::finalize ()
{
	if (_frame_buffer_offset > 0) {
		write_current_frame ();
	}
	
	if (ASDCP_FAILURE (_state->mxf_writer.Finalize())) {
		boost::throw_exception (MiscError ("could not finalise audio MXF"));
	}

	MXFWriter::finalize ();
}
