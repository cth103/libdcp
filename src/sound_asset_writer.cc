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


/** @file  src/sound_asset_writer.cc
 *  @brief SoundAssetWriter class
 */


#include "bitstream.h"
#include "compose.hpp"
#include "crypto_context.h"
#include "dcp_assert.h"
#include "exceptions.h"
#include "sound_asset.h"
#include "sound_asset_writer.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <asdcp/AS_DCP.h>
#include <asdcp/Metadata.h>
LIBDCP_ENABLE_WARNINGS
#include <iostream>


using std::min;
using std::max;
using std::cout;
using std::string;
using std::vector;
using namespace dcp;


struct SoundAssetWriter::ASDCPState
{
	ASDCP::PCM::MXFWriter mxf_writer;
	ASDCP::PCM::FrameBuffer frame_buffer;
	ASDCP::WriterInfo writer_info;
	ASDCP::PCM::AudioDescriptor desc;
};


SoundAssetWriter::SoundAssetWriter (SoundAsset* asset, boost::filesystem::path file, bool sync)
	: AssetWriter (asset, file)
	, _state (new SoundAssetWriter::ASDCPState)
	, _asset (asset)
	, _sync (sync)
{
	DCP_ASSERT (!_sync || _asset->channels() >= 14);
	DCP_ASSERT (!_sync || _asset->standard() == Standard::SMPTE);

	/* Derived from ASDCP::Wav::SimpleWaveHeader::FillADesc */
	_state->desc.EditRate = ASDCP::Rational (_asset->edit_rate().numerator, _asset->edit_rate().denominator);
	_state->desc.AudioSamplingRate = ASDCP::Rational (_asset->sampling_rate(), 1);
	_state->desc.Locked = 0;
	_state->desc.ChannelCount = _asset->channels();
	_state->desc.QuantizationBits = 24;
	_state->desc.BlockAlign = 3 * _asset->channels();
	_state->desc.AvgBps = _asset->sampling_rate() * _state->desc.BlockAlign;
	_state->desc.LinkedTrackID = 0;
	if (asset->standard() == Standard::INTEROP) {
		_state->desc.ChannelFormat = ASDCP::PCM::CF_NONE;
	} else {
		/* As required by Bv2.1 */
		_state->desc.ChannelFormat = ASDCP::PCM::CF_CFG_4;
	}

	/* I'm fairly sure this is not necessary, as ContainerDuration is written
	   in ASDCP's WriteMXFFooter, but it stops a valgrind warning.
	*/
	_state->desc.ContainerDuration = 0;

	_state->frame_buffer.Capacity (ASDCP::PCM::CalcFrameBufferSize (_state->desc));
	_state->frame_buffer.Size (ASDCP::PCM::CalcFrameBufferSize (_state->desc));
	memset (_state->frame_buffer.Data(), 0, _state->frame_buffer.Capacity());

	_asset->fill_writer_info (&_state->writer_info, _asset->id());

	if (_sync) {
		_fsk.set_data (create_sync_packets());
	}
}


void
SoundAssetWriter::start ()
{
	auto r = _state->mxf_writer.OpenWrite (_file.string().c_str(), _state->writer_info, _state->desc);
	if (ASDCP_FAILURE(r)) {
		boost::throw_exception (FileError("could not open audio MXF for writing", _file.string(), r));
	}

	if (_asset->standard() == Standard::SMPTE) {

		ASDCP::MXF::WaveAudioDescriptor* essence_descriptor = nullptr;
		_state->mxf_writer.OP1aHeader().GetMDObjectByType(
			asdcp_smpte_dict->ul(ASDCP::MDD_WaveAudioDescriptor), reinterpret_cast<ASDCP::MXF::InterchangeObject**>(&essence_descriptor)
			);
		DCP_ASSERT (essence_descriptor);
		essence_descriptor->ChannelAssignment = asdcp_smpte_dict->ul(ASDCP::MDD_DCAudioChannelCfg_4_WTF);

		auto soundfield = new ASDCP::MXF::SoundfieldGroupLabelSubDescriptor(asdcp_smpte_dict);
		GenRandomValue (soundfield->MCALinkID);
		if (auto lang = _asset->language()) {
			soundfield->RFC5646SpokenLanguage = *lang;
		}

		const MCASoundField field = _asset->channels() > 10 ? MCASoundField::SEVEN_POINT_ONE : MCASoundField::FIVE_POINT_ONE;

		if (field == MCASoundField::SEVEN_POINT_ONE) {
			soundfield->MCATagSymbol = "sg71";
			soundfield->MCATagName = "7.1DS";
LIBDCP_DISABLE_WARNINGS
			soundfield->MCALabelDictionaryID = asdcp_smpte_dict->ul(ASDCP::MDD_DCAudioSoundfield_71);
LIBDCP_ENABLE_WARNINGS
		} else {
			soundfield->MCATagSymbol = "sg51";
			soundfield->MCATagName = "5.1";
LIBDCP_DISABLE_WARNINGS
			soundfield->MCALabelDictionaryID = asdcp_smpte_dict->ul(ASDCP::MDD_DCAudioSoundfield_51);
LIBDCP_ENABLE_WARNINGS
		}

		_state->mxf_writer.OP1aHeader().AddChildObject(soundfield);
		essence_descriptor->SubDescriptors.push_back(soundfield->InstanceUID);

		/* We must describe at least the number of channels in `field', even if they aren't
		 * in the asset (I think)
		 */
		int descriptors = max(_asset->channels(), field == MCASoundField::FIVE_POINT_ONE ? 6 : 8);

		auto const used = used_audio_channels();

		for (auto i = 0; i < descriptors; ++i) {
			auto dcp_channel = static_cast<dcp::Channel>(i);
			if (find(used.begin(), used.end(), dcp_channel) == used.end()) {
				continue;
			}
			auto channel = new ASDCP::MXF::AudioChannelLabelSubDescriptor(asdcp_smpte_dict);
			GenRandomValue (channel->MCALinkID);
			channel->SoundfieldGroupLinkID = soundfield->MCALinkID;
			channel->MCAChannelID = i + 1;
			channel->MCATagSymbol = "ch" + channel_to_mca_id(dcp_channel, field);
			channel->MCATagName = channel_to_mca_name(dcp_channel, field);
			if (auto lang = _asset->language()) {
				channel->RFC5646SpokenLanguage = *lang;
			}
LIBDCP_DISABLE_WARNINGS
			channel->MCALabelDictionaryID = channel_to_mca_universal_label(dcp_channel, field, asdcp_smpte_dict);
LIBDCP_ENABLE_WARNINGS
			_state->mxf_writer.OP1aHeader().AddChildObject(channel);
			essence_descriptor->SubDescriptors.push_back(channel->InstanceUID);
		}
	}

	_asset->set_file (_file);
	_started = true;
}


void
SoundAssetWriter::write (float const * const * data, int frames)
{
	DCP_ASSERT (!_finalized);
	DCP_ASSERT (frames > 0);

	static float const clip = 1.0f - (1.0f / pow (2, 23));

	if (!_started) {
		start ();
	}

	int const ch = _asset->channels ();

	for (int i = 0; i < frames; ++i) {

		byte_t* out = _state->frame_buffer.Data() + _frame_buffer_offset;

		/* Write one sample per channel */
		for (int j = 0; j < ch; ++j) {
			int32_t s = 0;
			if (j == 13 && _sync) {
				s = _fsk.get();
			} else {
				/* Convert sample to 24-bit int, clipping if necessary. */
				float x = data[j][i];
				if (x > clip) {
					x = clip;
				} else if (x < -clip) {
					x = -clip;
				}
				s = x * (1 << 23);
			}
			*out++ = (s & 0xff);
			*out++ = (s & 0xff00) >> 8;
			*out++ = (s & 0xff0000) >> 16;
		}
		_frame_buffer_offset += 3 * ch;

		DCP_ASSERT (_frame_buffer_offset <= int(_state->frame_buffer.Capacity()));

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
	auto const r = _state->mxf_writer.WriteFrame (_state->frame_buffer, _crypto_context->context(), _crypto_context->hmac());
	if (ASDCP_FAILURE(r)) {
		boost::throw_exception (MiscError(String::compose("could not write audio MXF frame (%1)", static_cast<int>(r))));
	}

	++_frames_written;

	if (_sync) {
		/* We need a new set of sync packets for this frame */
		_fsk.set_data (create_sync_packets());
	}
}

bool
SoundAssetWriter::finalize ()
{
	if (_frame_buffer_offset > 0) {
		write_current_frame ();
	}

	if (_started) {
		auto const r = _state->mxf_writer.Finalize();
		if (ASDCP_FAILURE(r)) {
			boost::throw_exception (MiscError(String::compose ("could not finalise audio MXF (%1)", static_cast<int>(r))));
		}
	}

	_asset->_intrinsic_duration = _frames_written;
	return AssetWriter::finalize ();
}


/** Calculate and return the sync packets required for this edit unit (aka "frame") */
vector<bool>
SoundAssetWriter::create_sync_packets ()
{
	/* Parts of this code assumes 48kHz */
	DCP_ASSERT (_asset->sampling_rate() == 48000);

	/* Encoding of edit rate */
	int edit_rate_code = 0;
	/* How many 0 bits are used to pad the end of the packet */
	int remaining_bits = 0;
	/* How many packets in this edit unit (i.e. "frame") */
	int packets = 0;
	auto const edit_rate = _asset->edit_rate ();
	if (edit_rate == Fraction(24, 1)) {
		edit_rate_code = 0;
		remaining_bits = 25;
		packets = 4;
	} else if (edit_rate == Fraction(25, 1)) {
		edit_rate_code = 1;
		remaining_bits = 20;
		packets = 4;
	} else if (edit_rate == Fraction(30, 1)) {
		edit_rate_code = 2;
		remaining_bits = 0;
		packets = 4;
	} else if (edit_rate == Fraction(48, 1)) {
		edit_rate_code = 3;
		remaining_bits = 25;
		packets = 2;
	} else if (edit_rate == Fraction(50, 1)) {
		edit_rate_code = 4;
		remaining_bits = 20;
		packets = 2;
	} else if (edit_rate == Fraction(60, 1)) {
		edit_rate_code = 5;
		remaining_bits = 0;
		packets = 2;
	} else if (edit_rate == Fraction(96, 1)) {
		edit_rate_code = 6;
		remaining_bits = 25;
		packets = 1;
	} else if (edit_rate == Fraction(100, 1)) {
		edit_rate_code = 7;
		remaining_bits = 20;
		packets = 1;
	} else if (edit_rate == Fraction(120, 1)) {
		edit_rate_code = 8;
		remaining_bits = 0;
		packets = 1;
	}

	Bitstream bs;

	Kumu::UUID id;
	DCP_ASSERT (id.DecodeHex(_asset->id().c_str()));

	for (int i = 0; i < packets; ++i) {
		bs.write_from_byte (0x4d);
		bs.write_from_byte (0x56);
		bs.start_crc (0x1021);
		bs.write_from_byte (edit_rate_code, 4);
		bs.write_from_byte (0, 2);
		bs.write_from_byte (_sync_packet, 2);
		bs.write_from_byte (id.Value()[i * 4 + 0]);
		bs.write_from_byte (id.Value()[i * 4 + 1]);
		bs.write_from_byte (id.Value()[i * 4 + 2]);
		bs.write_from_byte (id.Value()[i * 4 + 3]);
		bs.write_from_word (_frames_written, 24);
		bs.write_crc ();
		bs.write_from_byte (0, 4);
		bs.write_from_word (0, remaining_bits);

		++_sync_packet;
		if (_sync_packet == 4) {
			_sync_packet = 0;
		}
	}

	return bs.get();
}

