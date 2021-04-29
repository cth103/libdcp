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


/** @file  src/sound_asset.cc
 *  @brief SoundAsset class
 */


#include "compose.hpp"
#include "dcp_assert.h"
#include "exceptions.h"
#include "sound_asset.h"
#include "sound_asset_reader.h"
#include "sound_asset_writer.h"
#include "sound_frame.h"
#include "util.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_fileio.h>
#include <asdcp/Metadata.h>
LIBDCP_ENABLE_WARNINGS
#include <libxml++/nodes/element.h>
#include <boost/filesystem.hpp>
#include <stdexcept>


using std::string;
using std::vector;
using std::list;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using namespace dcp;


SoundAsset::SoundAsset (boost::filesystem::path file)
	: Asset (file)
{
	ASDCP::PCM::MXFReader reader;
	auto r = reader.OpenRead (file.string().c_str());
	if (ASDCP_FAILURE(r)) {
		boost::throw_exception (MXFFileError("could not open MXF file for reading", file.string(), r));
	}

	ASDCP::PCM::AudioDescriptor desc;
	if (ASDCP_FAILURE (reader.FillAudioDescriptor(desc))) {
		boost::throw_exception (ReadError("could not read audio MXF information"));
	}

	_sampling_rate = desc.AudioSamplingRate.Numerator / desc.AudioSamplingRate.Denominator;
	_channels = desc.ChannelCount;
	_edit_rate = Fraction (desc.EditRate.Numerator, desc.EditRate.Denominator);

	_intrinsic_duration = desc.ContainerDuration;

	ASDCP::WriterInfo info;
	if (ASDCP_FAILURE (reader.FillWriterInfo(info))) {
		boost::throw_exception (ReadError("could not read audio MXF information"));
	}

	ASDCP::MXF::SoundfieldGroupLabelSubDescriptor* soundfield;
	auto rr = reader.OP1aHeader().GetMDObjectByType(
		asdcp_smpte_dict->ul(ASDCP::MDD_SoundfieldGroupLabelSubDescriptor),
		reinterpret_cast<ASDCP::MXF::InterchangeObject**>(&soundfield)
		);

	if (KM_SUCCESS(rr)) {
		if (!soundfield->RFC5646SpokenLanguage.empty()) {
			char buffer[64];
			soundfield->RFC5646SpokenLanguage.get().EncodeString(buffer, sizeof(buffer));
			_language = buffer;
		}
	}

	_id = read_writer_info (info);
}


SoundAsset::SoundAsset (Fraction edit_rate, int sampling_rate, int channels, LanguageTag language, Standard standard)
	: MXF (standard)
	, _edit_rate (edit_rate)
	, _channels (channels)
	, _sampling_rate (sampling_rate)
	, _language (language.to_string())
{

}


bool
SoundAsset::equals (shared_ptr<const Asset> other, EqualityOptions opt, NoteHandler note) const
{
	ASDCP::PCM::MXFReader reader_A;
	DCP_ASSERT (file());
	auto r = reader_A.OpenRead (file()->string().c_str());
	if (ASDCP_FAILURE(r)) {
		boost::throw_exception (MXFFileError("could not open MXF file for reading", file()->string(), r));
	}

	ASDCP::PCM::MXFReader reader_B;
	r = reader_B.OpenRead (other->file()->string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError("could not open MXF file for reading", other->file()->string(), r));
	}

	ASDCP::PCM::AudioDescriptor desc_A;
	if (ASDCP_FAILURE (reader_A.FillAudioDescriptor(desc_A))) {
		boost::throw_exception (ReadError ("could not read audio MXF information"));
	}
	ASDCP::PCM::AudioDescriptor desc_B;
	if (ASDCP_FAILURE (reader_B.FillAudioDescriptor(desc_B))) {
		boost::throw_exception (ReadError ("could not read audio MXF information"));
	}

	if (desc_A.EditRate != desc_B.EditRate) {
		note (
			NoteType::ERROR,
			String::compose (
				"audio edit rates differ: %1/%2 cf %3/%4",
				desc_A.EditRate.Numerator, desc_A.EditRate.Denominator, desc_B.EditRate.Numerator, desc_B.EditRate.Denominator
				)
			);
		return false;
	} else if (desc_A.AudioSamplingRate != desc_B.AudioSamplingRate) {
		note (
			NoteType::ERROR,
			String::compose (
				"audio sampling rates differ: %1 cf %2",
				desc_A.AudioSamplingRate.Numerator, desc_A.AudioSamplingRate.Denominator,
				desc_B.AudioSamplingRate.Numerator, desc_B.AudioSamplingRate.Numerator
				)
			);
		return false;
	} else if (desc_A.Locked != desc_B.Locked) {
		note (NoteType::ERROR, String::compose ("audio locked flags differ: %1 cf %2", desc_A.Locked, desc_B.Locked));
		return false;
	} else if (desc_A.ChannelCount != desc_B.ChannelCount) {
		note (NoteType::ERROR, String::compose ("audio channel counts differ: %1 cf %2", desc_A.ChannelCount, desc_B.ChannelCount));
		return false;
	} else if (desc_A.QuantizationBits != desc_B.QuantizationBits) {
		note (NoteType::ERROR, String::compose ("audio bits per sample differ: %1 cf %2", desc_A.QuantizationBits, desc_B.QuantizationBits));
		return false;
	} else if (desc_A.BlockAlign != desc_B.BlockAlign) {
		note (NoteType::ERROR, String::compose ("audio bytes per sample differ: %1 cf %2", desc_A.BlockAlign, desc_B.BlockAlign));
		return false;
	} else if (desc_A.AvgBps != desc_B.AvgBps) {
		note (NoteType::ERROR, String::compose ("audio average bps differ: %1 cf %2", desc_A.AvgBps, desc_B.AvgBps));
		return false;
	} else if (desc_A.LinkedTrackID != desc_B.LinkedTrackID) {
		note (NoteType::ERROR, String::compose ("audio linked track IDs differ: %1 cf %2", desc_A.LinkedTrackID, desc_B.LinkedTrackID));
		return false;
	} else if (desc_A.ContainerDuration != desc_B.ContainerDuration) {
		note (NoteType::ERROR, String::compose ("audio container durations differ: %1 cf %2", desc_A.ContainerDuration, desc_B.ContainerDuration));
		return false;
	} else if (desc_A.ChannelFormat != desc_B.ChannelFormat) {
		/* XXX */
	}

	auto other_sound = dynamic_pointer_cast<const SoundAsset> (other);

	auto reader = start_read ();
	auto other_reader = other_sound->start_read ();

	for (int i = 0; i < _intrinsic_duration; ++i) {

		auto frame_A = reader->get_frame (i);
		auto frame_B = other_reader->get_frame (i);

		if (frame_A->size() != frame_B->size()) {
			note (NoteType::ERROR, String::compose ("sizes of audio data for frame %1 differ", i));
			return false;
		}

		if (memcmp (frame_A->data(), frame_B->data(), frame_A->size()) != 0) {
			for (int sample = 0; sample < frame_A->samples(); ++sample) {
				for (int channel = 0; channel < frame_A->channels(); ++channel) {
					int32_t const d = abs(frame_A->get(channel, sample) - frame_B->get(channel, sample));
					if (d > opt.max_audio_sample_error) {
						note (NoteType::ERROR, String::compose("PCM data difference of %1 in frame %2, channel %3, sample %4", d, i, channel, sample));
						return false;
					}
				}
			}
		}
	}

	return true;
}


shared_ptr<SoundAssetWriter>
SoundAsset::start_write (boost::filesystem::path file, bool atmos_sync)
{
	if (atmos_sync && _channels < 14) {
		throw MiscError ("Insufficient channels to write ATMOS sync (there must be at least 14)");
	}

	return shared_ptr<SoundAssetWriter> (new SoundAssetWriter(this, file, atmos_sync));
}


shared_ptr<SoundAssetReader>
SoundAsset::start_read () const
{
	return shared_ptr<SoundAssetReader> (new SoundAssetReader(this, key(), standard()));
}


string
SoundAsset::static_pkl_type (Standard standard)
{
	switch (standard) {
	case Standard::INTEROP:
		return "application/x-smpte-mxf;asdcpKind=Sound";
	case Standard::SMPTE:
		return "application/mxf";
	default:
		DCP_ASSERT (false);
	}
}


bool
SoundAsset::valid_mxf (boost::filesystem::path file)
{
	ASDCP::PCM::MXFReader reader;
	Kumu::Result_t r = reader.OpenRead (file.string().c_str());
	return !ASDCP_FAILURE (r);
}
