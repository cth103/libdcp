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

/** @file  src/sound_mxf.cc
 *  @brief SoundMXF class.
 */

#include "sound_mxf.h"
#include "util.h"
#include "exceptions.h"
#include "sound_frame.h"
#include "sound_mxf_writer.h"
#include "compose.hpp"
#include "KM_fileio.h"
#include "AS_DCP.h"
#include <libxml++/nodes/element.h>
#include <boost/filesystem.hpp>
#include <iostream>
#include <stdexcept>

using std::string;
using std::stringstream;
using std::ostream;
using std::vector;
using std::list;
using boost::shared_ptr;
using namespace dcp;

SoundMXF::SoundMXF (boost::filesystem::path file)
	: MXF (file)
	, _channels (0)
	, _sampling_rate (0)
{
	ASDCP::PCM::MXFReader reader;
	Kumu::Result_t r = reader.OpenRead (file.string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", file.string(), r));
	}

	ASDCP::PCM::AudioDescriptor desc;
	if (ASDCP_FAILURE (reader.FillAudioDescriptor (desc))) {
		boost::throw_exception (DCPReadError ("could not read audio MXF information"));
	}

	_sampling_rate = desc.AudioSamplingRate.Numerator / desc.AudioSamplingRate.Denominator;
	_channels = desc.ChannelCount;
	_edit_rate = Fraction (desc.EditRate.Numerator, desc.EditRate.Denominator);

	_intrinsic_duration = desc.ContainerDuration;

	ASDCP::WriterInfo info;
	if (ASDCP_FAILURE (reader.FillWriterInfo (info))) {
		boost::throw_exception (DCPReadError ("could not read audio MXF information"));
	}

	read_writer_info (info);

}

SoundMXF::SoundMXF (Fraction edit_rate, int sampling_rate, int channels)
	: MXF (edit_rate)
	, _channels (channels)
	, _sampling_rate (sampling_rate)
{

}

bool
SoundMXF::equals (shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (!MXF::equals (other, opt, note)) {
		return false;
	}
		     
	ASDCP::PCM::MXFReader reader_A;
	Kumu::Result_t r = reader_A.OpenRead (file().string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", file().string(), r));
	}

	ASDCP::PCM::MXFReader reader_B;
	r = reader_B.OpenRead (other->file().string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", file().string(), r));
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
		
		note (DCP_ERROR, "audio MXF picture descriptors differ");
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
			note (DCP_ERROR, String::compose ("sizes of audio data for frame %1 differ", i));
			return false;
		}
		
		if (memcmp (buffer_A.RoData(), buffer_B.RoData(), buffer_A.Size()) != 0) {
			for (uint32_t i = 0; i < buffer_A.Size(); ++i) {
				int const d = abs (buffer_A.RoData()[i] - buffer_B.RoData()[i]);
				if (d > opt.max_audio_sample_error) {
					note (DCP_ERROR, String::compose ("PCM data difference of %1", d));
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
	return shared_ptr<const SoundFrame> (new SoundFrame (file(), n, _decryption_context));
}

shared_ptr<SoundMXFWriter>
SoundMXF::start_write (boost::filesystem::path file, Standard standard)
{
	/* XXX: can't we use a shared_ptr here? */
	return shared_ptr<SoundMXFWriter> (new SoundMXFWriter (this, file, standard));
}
