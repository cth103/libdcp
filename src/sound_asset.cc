/*
    Copyright (C) 2012-2016 Carl Hetherington <cth@carlh.net>

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

*/

/** @file  src/sound_mxf.cc
 *  @brief SoundAsset class.
 */

#include "sound_asset.h"
#include "util.h"
#include "exceptions.h"
#include "sound_frame.h"
#include "sound_asset_writer.h"
#include "sound_asset_reader.h"
#include "compose.hpp"
#include "KM_fileio.h"
#include "AS_DCP.h"
#include "dcp_assert.h"
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
using boost::dynamic_pointer_cast;
using namespace dcp;

SoundAsset::SoundAsset (boost::filesystem::path file)
	: Asset (file)
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

	_id = read_writer_info (info);
}

SoundAsset::SoundAsset (Fraction edit_rate, int sampling_rate, int channels)
	: _edit_rate (edit_rate)
	, _intrinsic_duration (0)
	, _channels (channels)
	, _sampling_rate (sampling_rate)
{

}

bool
SoundAsset::equals (shared_ptr<const Asset> other, EqualityOptions opt, NoteHandler note) const
{
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

	shared_ptr<const SoundAsset> other_sound = dynamic_pointer_cast<const SoundAsset> (other);

	shared_ptr<const SoundAssetReader> reader = start_read ();
	shared_ptr<const SoundAssetReader> other_reader = other_sound->start_read ();

	for (int i = 0; i < _intrinsic_duration; ++i) {

		shared_ptr<const SoundFrame> frame_A = reader->get_frame (i);
		shared_ptr<const SoundFrame> frame_B = other_reader->get_frame (i);

		if (frame_A->size() != frame_B->size()) {
			note (DCP_ERROR, String::compose ("sizes of audio data for frame %1 differ", i));
			return false;
		}

		if (memcmp (frame_A->data(), frame_B->data(), frame_A->size()) != 0) {
			for (int i = 0; i < frame_A->size(); ++i) {
				int const d = abs (frame_A->data()[i] - frame_B->data()[i]);
				if (d > opt.max_audio_sample_error) {
					note (DCP_ERROR, String::compose ("PCM data difference of %1", d));
					return false;
				}
			}
		}
	}

	return true;
}

shared_ptr<SoundAssetWriter>
SoundAsset::start_write (boost::filesystem::path file, Standard standard)
{
	/* XXX: can't we use a shared_ptr here? */
	return shared_ptr<SoundAssetWriter> (new SoundAssetWriter (this, file, standard));
}

shared_ptr<SoundAssetReader>
SoundAsset::start_read () const
{
	return shared_ptr<SoundAssetReader> (new SoundAssetReader (this));
}

string
SoundAsset::pkl_type (Standard standard) const
{
	switch (standard) {
	case INTEROP:
		return "application/x-smpte-mxf;asdcpKind=Sound";
	case SMPTE:
		return "application/mxf";
	default:
		DCP_ASSERT (false);
	}
}

bool
SoundAsset::valid_mxf (boost::filesystem::path file)
{
	ASDCP::PCM::MXFReader reader;
	Kumu::Result_t r = reader.OpenRead (file.string().c_str ());
	return !ASDCP_FAILURE (r);
}
