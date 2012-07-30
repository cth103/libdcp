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
#include "AS_DCP.h"
#include "sound_asset.h"
#include "util.h"
#include "exceptions.h"

using namespace std;
using namespace boost;
using namespace libdcp;

SoundAsset::SoundAsset (
	vector<string> const & files, string mxf_path, sigc::signal1<void, float>* progress, int fps, int length
	)
	: Asset (mxf_path, progress, fps, length)
	, _channels (files.size ())
{
	construct (sigc::bind (sigc::mem_fun (*this, &SoundAsset::path_from_channel), files));
}

SoundAsset::SoundAsset (
	sigc::slot<string, Channel> get_path, string mxf_path, sigc::signal1<void, float>* progress, int fps, int length, int channels
	)
	: Asset (mxf_path, progress, fps, length)
	, _channels (channels)
{
	construct (get_path);
}

SoundAsset::SoundAsset (string mxf_path, int fps, int length)
	: Asset (mxf_path, 0, fps, length)
	, _channels (0)
{

}

string
SoundAsset::path_from_channel (Channel channel, vector<string> const & files)
{
	unsigned int const c = int (channel);
	assert (c < files.size ());
	return files[c];
}

void
SoundAsset::construct (sigc::slot<string, Channel> get_path)
{
	ASDCP::Rational asdcp_fps (_fps, 1);
	
 	ASDCP::PCM::WAVParser pcm_parser_channel[_channels];
	if (pcm_parser_channel[0].OpenRead (get_path(LEFT).c_str(), asdcp_fps)) {
		throw FileError ("could not open WAV file for reading", get_path(LEFT));
	}
	
	ASDCP::PCM::AudioDescriptor audio_desc;
	pcm_parser_channel[0].FillAudioDescriptor (audio_desc);
	audio_desc.ChannelCount = 0;
	audio_desc.BlockAlign = 0;
	audio_desc.EditRate = asdcp_fps;
	audio_desc.AvgBps = audio_desc.AvgBps * _channels;

	Channel channels[] = {
		LEFT,
		RIGHT,
		CENTRE,
		LFE,
		LS,
		RS
	};

	ASDCP::PCM::FrameBuffer frame_buffer_channel[_channels];
	ASDCP::PCM::AudioDescriptor audio_desc_channel[_channels];

	for (int i = 0; i < _channels; ++i) {

		string const path = get_path (channels[i]);
		
		if (ASDCP_FAILURE (pcm_parser_channel[i].OpenRead (path.c_str(), asdcp_fps))) {
			throw FileError ("could not open WAV file for reading", path);
		}

		pcm_parser_channel[i].FillAudioDescriptor (audio_desc_channel[i]);
		frame_buffer_channel[i].Capacity (ASDCP::PCM::CalcFrameBufferSize (audio_desc_channel[i]));

		audio_desc.ChannelCount += audio_desc_channel[i].ChannelCount;
		audio_desc.BlockAlign += audio_desc_channel[i].BlockAlign;
	}

	ASDCP::PCM::FrameBuffer frame_buffer;
	frame_buffer.Capacity (ASDCP::PCM::CalcFrameBufferSize (audio_desc));
	frame_buffer.Size (ASDCP::PCM::CalcFrameBufferSize (audio_desc));

	ASDCP::WriterInfo writer_info;
	fill_writer_info (&writer_info);

	ASDCP::PCM::MXFWriter mxf_writer;
	if (ASDCP_FAILURE (mxf_writer.OpenWrite (_mxf_path.c_str(), writer_info, audio_desc))) {
		throw FileError ("could not open audio MXF for writing", _mxf_path);
	}

	for (int i = 0; i < _length; ++i) {

		byte_t *data_s = frame_buffer.Data();
		byte_t *data_e = data_s + frame_buffer.Capacity();
		byte_t sample_size = ASDCP::PCM::CalcSampleSize (audio_desc_channel[0]);
		int offset = 0;

		for (int j = 0; j < _channels; ++j) {
			memset (frame_buffer_channel[j].Data(), 0, frame_buffer_channel[j].Capacity());
			if (ASDCP_FAILURE (pcm_parser_channel[j].ReadFrame (frame_buffer_channel[j]))) {
				throw MiscError ("could not read audio frame");
			}
			
			if (frame_buffer_channel[j].Size() != frame_buffer_channel[j].Capacity()) {
				throw MiscError ("short audio frame");
			}
		}

		while (data_s < data_e) {
			for (int j = 0; j < _channels; ++j) {
				byte_t* frame = frame_buffer_channel[j].Data() + offset;
				memcpy (data_s, frame, sample_size);
				data_s += sample_size;
			}
			offset += sample_size;
		}

		if (ASDCP_FAILURE (mxf_writer.WriteFrame (frame_buffer, 0, 0))) {
			throw MiscError ("could not write audio MXF frame");
		}

		(*_progress) (0.5 * float (i) / _length);
	}

	if (ASDCP_FAILURE (mxf_writer.Finalize())) {
		throw MiscError ("could not finalise audio MXF");
	}

	_digest = make_digest (_mxf_path, _progress);
}

void
SoundAsset::write_to_cpl (ostream& s) const
{
	s << "        <MainSound>\n"
	  << "          <Id>urn:uuid:" << _uuid << "</Id>\n"
#if BOOST_FILESYSTEM_VERSION == 3		
	  << "          <AnnotationText>" << filesystem::path(_mxf_path).filename().string() << "</AnnotationText>\n"
#else		
	  << "          <AnnotationText>" << filesystem::path(_mxf_path).filename() << "</AnnotationText>\n"
#endif		
	  << "          <EditRate>" << _fps << " 1</EditRate>\n"
	  << "          <IntrinsicDuration>" << _length << "</IntrinsicDuration>\n"
	  << "          <EntryPoint>0</EntryPoint>\n"
	  << "          <Duration>" << _length << "</Duration>\n"
	  << "        </MainSound>\n";
}

