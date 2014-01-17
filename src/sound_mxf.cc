/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

/** @file  src/sound_asset.cc
 *  @brief An asset made up of WAV files
 */

#include <iostream>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <libxml++/nodes/element.h>
#include "KM_fileio.h"
#include "AS_DCP.h"
#include "sound_mxf.h"
#include "util.h"
#include "exceptions.h"
#include "sound_frame.h"

using std::string;
using std::stringstream;
using std::ostream;
using std::vector;
using std::list;
using boost::shared_ptr;
using boost::lexical_cast;
using namespace dcp;

SoundMXF::SoundMXF (boost::filesystem::path directory, boost::filesystem::path mxf_name)
	: MXF (directory, mxf_name)
	, _channels (0)
	, _sampling_rate (0)
{

}

void
SoundMXF::read ()
{
	ASDCP::PCM::MXFReader reader;
	Kumu::Result_t r = reader.OpenRead (path().string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", path().string(), r));
	}

	ASDCP::PCM::AudioDescriptor desc;
	if (ASDCP_FAILURE (reader.FillAudioDescriptor (desc))) {
		boost::throw_exception (DCPReadError ("could not read audio MXF information"));
	}

	_sampling_rate = desc.AudioSamplingRate.Numerator / desc.AudioSamplingRate.Denominator;
	_channels = desc.ChannelCount;
	_edit_rate = desc.EditRate.Numerator;
	assert (desc.EditRate.Denominator == 1);
	_intrinsic_duration = desc.ContainerDuration;
}

string
SoundMXF::cpl_node_name () const
{
	return "MainSound";
}

bool
SoundMXF::equals (shared_ptr<const ContentAsset> other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (!MXF::equals (other, opt, note)) {
		return false;
	}
		     
	ASDCP::PCM::MXFReader reader_A;
	Kumu::Result_t r = reader_A.OpenRead (path().string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", path().string(), r));
	}

	ASDCP::PCM::MXFReader reader_B;
	r = reader_B.OpenRead (other->path().string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", path().string(), r));
	}

	ASDCP::PCM::AudioDescriptor desc_A;
	if (ASDCP_FAILURE (reader_A.FillAudioDescriptor (desc_A))) {
		boost::throw_exception (DCPReadError ("could not read audio MXF information"));
	}
	ASDCP::PCM::AudioDescriptor desc_B;
	if (ASDCP_FAILURE (reader_B.FillAudioDescriptor (desc_B))) {
		boost::throw_exception (DCPReadError ("could not read audio MXF information"));
	}
	
	if (
		desc_A.EditRate != desc_B.EditRate ||
		desc_A.AudioSamplingRate != desc_B.AudioSamplingRate ||
		desc_A.Locked != desc_B.Locked ||
		desc_A.ChannelCount != desc_B.ChannelCount ||
		desc_A.QuantizationBits != desc_B.QuantizationBits ||
		desc_A.BlockAlign != desc_B.BlockAlign ||
		desc_A.AvgBps != desc_B.AvgBps ||
		desc_A.LinkedTrackID != desc_B.LinkedTrackID ||
		desc_A.ContainerDuration != desc_B.ContainerDuration
//		desc_A.ChannelFormat != desc_B.ChannelFormat ||
		) {
		
		note (ERROR, "audio MXF picture descriptors differ");
		return false;
	}
	
	ASDCP::PCM::FrameBuffer buffer_A (1 * Kumu::Megabyte);
	ASDCP::PCM::FrameBuffer buffer_B (1 * Kumu::Megabyte);
	
	for (int i = 0; i < _intrinsic_duration; ++i) {
		if (ASDCP_FAILURE (reader_A.ReadFrame (i, buffer_A))) {
			boost::throw_exception (DCPReadError ("could not read audio frame"));
		}
		
		if (ASDCP_FAILURE (reader_B.ReadFrame (i, buffer_B))) {
			boost::throw_exception (DCPReadError ("could not read audio frame"));
		}
		
		if (buffer_A.Size() != buffer_B.Size()) {
			note (ERROR, "sizes of audio data for frame " + lexical_cast<string>(i) + " differ");
			return false;
		}
		
		if (memcmp (buffer_A.RoData(), buffer_B.RoData(), buffer_A.Size()) != 0) {
			for (uint32_t i = 0; i < buffer_A.Size(); ++i) {
				int const d = abs (buffer_A.RoData()[i] - buffer_B.RoData()[i]);
				if (d > opt.max_audio_sample_error) {
					note (ERROR, "PCM data difference of " + lexical_cast<string> (d));
					return false;
				}
			}
		}
	}

	return true;
}

shared_ptr<const SoundFrame>
SoundMXF::get_frame (int n) const
{
	/* XXX: should add on entry point here? */
	return shared_ptr<const SoundFrame> (new SoundFrame (path().string(), n, _decryption_context));
}

shared_ptr<SoundMXFWriter>
SoundMXF::start_write ()
{
	/* XXX: can't we use a shared_ptr here? */
	return shared_ptr<SoundMXFWriter> (new SoundMXFWriter (this));
}

struct SoundMXFWriter::ASDCPState
{
	ASDCP::PCM::MXFWriter mxf_writer;
	ASDCP::PCM::FrameBuffer frame_buffer;
	ASDCP::WriterInfo writer_info;
	ASDCP::PCM::AudioDescriptor audio_desc;
	ASDCP::AESEncContext* encryption_context;
};

SoundMXFWriter::SoundMXFWriter (SoundMXF* a)
	: _state (new SoundMXFWriter::ASDCPState)
	, _asset (a)
	, _finalized (false)
	, _frames_written (0)
	, _frame_buffer_offset (0)
{
	_state->encryption_context = a->encryption_context ();
	
	/* Derived from ASDCP::Wav::SimpleWaveHeader::FillADesc */
	_state->audio_desc.EditRate = ASDCP::Rational (_asset->edit_rate(), 1);
	_state->audio_desc.AudioSamplingRate = ASDCP::Rational (_asset->sampling_rate(), 1);
	_state->audio_desc.Locked = 0;
	_state->audio_desc.ChannelCount = _asset->channels ();
	_state->audio_desc.QuantizationBits = 24;
	_state->audio_desc.BlockAlign = 3 * _asset->channels();
	_state->audio_desc.AvgBps = _asset->sampling_rate() * _state->audio_desc.BlockAlign;
	_state->audio_desc.LinkedTrackID = 0;
	_state->audio_desc.ChannelFormat = ASDCP::PCM::CF_NONE;
	
	_state->frame_buffer.Capacity (ASDCP::PCM::CalcFrameBufferSize (_state->audio_desc));
	_state->frame_buffer.Size (ASDCP::PCM::CalcFrameBufferSize (_state->audio_desc));
	memset (_state->frame_buffer.Data(), 0, _state->frame_buffer.Capacity());
	
	_asset->fill_writer_info (&_state->writer_info);
	
	Kumu::Result_t r = _state->mxf_writer.OpenWrite (_asset->path().string().c_str(), _state->writer_info, _state->audio_desc);
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (FileError ("could not open audio MXF for writing", _asset->path().string(), r));
	}
}

void
SoundMXFWriter::write (float const * const * data, int frames)
{
	for (int i = 0; i < frames; ++i) {

		byte_t* out = _state->frame_buffer.Data() + _frame_buffer_offset;

		/* Write one sample per channel */
		for (int j = 0; j < _asset->channels(); ++j) {
			int32_t const s = data[j][i] * (1 << 23);
			*out++ = (s & 0xff);
			*out++ = (s & 0xff00) >> 8;
			*out++ = (s & 0xff0000) >> 16;
		}
		_frame_buffer_offset += 3 * _asset->channels();

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
		boost::throw_exception (MiscError ("could not write audio MXF frame (" + lexical_cast<string> (int (r)) + ")"));
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

	_finalized = true;
	_asset->set_intrinsic_duration (_frames_written);
	_asset->set_duration (_frames_written);
}

string
SoundMXF::key_type () const
{
	return "MDAK";
}
