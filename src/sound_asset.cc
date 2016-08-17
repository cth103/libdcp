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
#include "dcp_assert.h"
#include <asdcp/KM_fileio.h>
#include <asdcp/AS_DCP.h>
#include <libxml++/nodes/element.h>
#include <boost/filesystem.hpp>
#include <stdexcept>

using std::string;
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
	DCP_ASSERT (file ());
	Kumu::Result_t r = reader_A.OpenRead (file()->string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", file()->string(), r));
	}

	ASDCP::PCM::MXFReader reader_B;
	r = reader_B.OpenRead (other->file()->string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", other->file()->string(), r));
	}

	ASDCP::PCM::AudioDescriptor desc_A;
	if (ASDCP_FAILURE (reader_A.FillAudioDescriptor (desc_A))) {
		boost::throw_exception (DCPReadError ("could not read audio MXF information"));
	}
	ASDCP::PCM::AudioDescriptor desc_B;
	if (ASDCP_FAILURE (reader_B.FillAudioDescriptor (desc_B))) {
		boost::throw_exception (DCPReadError ("could not read audio MXF information"));
	}

	if (desc_A.EditRate != desc_B.EditRate) {
		note (
			DCP_ERROR,
			String::compose (
				"audio edit rates differ: %1/%2 cf %3/%4",
				desc_A.EditRate.Numerator, desc_A.EditRate.Denominator, desc_B.EditRate.Numerator, desc_B.EditRate.Denominator
				)
			);
		return false;
	} else if (desc_A.AudioSamplingRate != desc_B.AudioSamplingRate) {
		note (
			DCP_ERROR,
			String::compose (
				"audio sampling rates differ: %1 cf %2",
				desc_A.AudioSamplingRate.Numerator, desc_A.AudioSamplingRate.Denominator,
				desc_B.AudioSamplingRate.Numerator, desc_B.AudioSamplingRate.Numerator
				)
			);
		return false;
	} else if (desc_A.Locked != desc_B.Locked) {
		note (DCP_ERROR, String::compose ("audio locked flags differ: %1 cf %2", desc_A.Locked, desc_B.Locked));
		return false;
	} else if (desc_A.ChannelCount != desc_B.ChannelCount) {
		note (DCP_ERROR, String::compose ("audio channel counts differ: %1 cf %2", desc_A.ChannelCount, desc_B.ChannelCount));
		return false;
	} else if (desc_A.QuantizationBits != desc_B.QuantizationBits) {
		note (DCP_ERROR, String::compose ("audio bits per sample differ: %1 cf %2", desc_A.QuantizationBits, desc_B.QuantizationBits));
		return false;
	} else if (desc_A.BlockAlign != desc_B.BlockAlign) {
		note (DCP_ERROR, String::compose ("audio bytes per sample differ: %1 cf %2", desc_A.BlockAlign, desc_B.BlockAlign));
		return false;
	} else if (desc_A.AvgBps != desc_B.AvgBps) {
		note (DCP_ERROR, String::compose ("audio average bps differ: %1 cf %2", desc_A.AvgBps, desc_B.AvgBps));
		return false;
	} else if (desc_A.LinkedTrackID != desc_B.LinkedTrackID) {
		note (DCP_ERROR, String::compose ("audio linked track IDs differ: %1 cf %2", desc_A.LinkedTrackID, desc_B.LinkedTrackID));
		return false;
	} else if (desc_A.ContainerDuration != desc_B.ContainerDuration) {
		note (DCP_ERROR, String::compose ("audio container durations differ: %1 cf %2", desc_A.ContainerDuration, desc_B.ContainerDuration));
		return false;
	} else if (desc_A.ChannelFormat != desc_B.ChannelFormat) {
		/* XXX */
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
