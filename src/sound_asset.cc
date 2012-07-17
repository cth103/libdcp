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

using namespace std;
using namespace boost;
using namespace libdcp;

SoundAsset::SoundAsset (list<string> const & files, string mxf_path, sigc::signal1<void, float>* progress, int fps, int length)
	: Asset (mxf_path, progress, fps, length)
{
	ASDCP::Rational asdcp_fps (_fps, 1);
	
 	ASDCP::PCM::WAVParser pcm_parser_channel[files.size()];
	if (pcm_parser_channel[0].OpenRead (files.front().c_str(), asdcp_fps)) {
		throw runtime_error ("could not open WAV file for reading");
	}
	
	ASDCP::PCM::AudioDescriptor audio_desc;
	pcm_parser_channel[0].FillAudioDescriptor (audio_desc);
	audio_desc.ChannelCount = 0;
	audio_desc.BlockAlign = 0;
	audio_desc.EditRate = asdcp_fps;
	audio_desc.AvgBps = audio_desc.AvgBps * files.size ();

	ASDCP::PCM::FrameBuffer frame_buffer_channel[files.size()];
	ASDCP::PCM::AudioDescriptor audio_desc_channel[files.size()];
	
	int j = 0;
	for (list<string>::const_iterator i = files.begin(); i != files.end(); ++i) {
		
		if (ASDCP_FAILURE (pcm_parser_channel[j].OpenRead (i->c_str(), asdcp_fps))) {
			throw runtime_error ("could not open WAV file for reading");
		}

		pcm_parser_channel[j].FillAudioDescriptor (audio_desc_channel[j]);
		frame_buffer_channel[j].Capacity (ASDCP::PCM::CalcFrameBufferSize (audio_desc_channel[j]));

		audio_desc.ChannelCount += audio_desc_channel[j].ChannelCount;
		audio_desc.BlockAlign += audio_desc_channel[j].BlockAlign;
		++j;
	}

	ASDCP::PCM::FrameBuffer frame_buffer;
	frame_buffer.Capacity (ASDCP::PCM::CalcFrameBufferSize (audio_desc));
	frame_buffer.Size (ASDCP::PCM::CalcFrameBufferSize (audio_desc));

	ASDCP::WriterInfo writer_info;
	fill_writer_info (&writer_info);

	ASDCP::PCM::MXFWriter mxf_writer;
	if (ASDCP_FAILURE (mxf_writer.OpenWrite (_mxf_path.c_str(), writer_info, audio_desc))) {
		throw runtime_error ("could not open audio MXF for writing");
	}

	for (int i = 0; i < _length; ++i) {

		byte_t *data_s = frame_buffer.Data();
		byte_t *data_e = data_s + frame_buffer.Capacity();
		byte_t sample_size = ASDCP::PCM::CalcSampleSize (audio_desc_channel[0]);
		int offset = 0;

		for (list<string>::size_type j = 0; j < files.size(); ++j) {
			memset (frame_buffer_channel[j].Data(), 0, frame_buffer_channel[j].Capacity());
			if (ASDCP_FAILURE (pcm_parser_channel[j].ReadFrame (frame_buffer_channel[j]))) {
				throw runtime_error ("could not read audio frame");
			}
			
			if (frame_buffer_channel[j].Size() != frame_buffer_channel[j].Capacity()) {
				throw runtime_error ("short audio frame");
			}
		}

		while (data_s < data_e) {
			for (list<string>::size_type j = 0; j < files.size(); ++j) {
				byte_t* frame = frame_buffer_channel[j].Data() + offset;
				memcpy (data_s, frame, sample_size);
				data_s += sample_size;
			}
			offset += sample_size;
		}

		if (ASDCP_FAILURE (mxf_writer.WriteFrame (frame_buffer, 0, 0))) {
			throw runtime_error ("could not write audio MXF frame");
		}

		(*_progress) (0.5 * float (i) / _length);
	}

	if (ASDCP_FAILURE (mxf_writer.Finalize())) {
		throw runtime_error ("could not finalise audio MXF");
	}

	_digest = make_digest (_mxf_path, _progress);
}

void
SoundAsset::write_to_cpl (ostream& s) const
{
	s << "        <MainSound>\n"
	  << "          <Id>urn:uuid:" << _uuid << "</Id>\n"
	  << "          <AnnotationText>" << filesystem::path(_mxf_path).filename() << "</AnnotationText>\n"
	  << "          <EditRate>" << _fps << " 1</EditRate>\n"
	  << "          <IntrinsicDuration>" << _length << "</IntrinsicDuration>\n"
	  << "          <EntryPoint>0</EntryPoint>\n"
	  << "          <Duration>" << _length << "</Duration>\n"
	  << "        </MainSound>\n";
}

