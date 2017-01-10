/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#include "sound_asset_writer.h"
#include "sound_asset.h"
#include "exceptions.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include "encryption_context.h"
#include <asdcp/AS_DCP.h>

using std::min;
using std::max;
using namespace dcp;

struct SoundAssetWriter::ASDCPState
{
	ASDCP::PCM::MXFWriter mxf_writer;
	ASDCP::PCM::FrameBuffer frame_buffer;
	ASDCP::WriterInfo writer_info;
	ASDCP::PCM::AudioDescriptor desc;
};

SoundAssetWriter::SoundAssetWriter (SoundAsset* asset, boost::filesystem::path file, Standard standard)
	: AssetWriter (asset, file, standard)
	, _state (new SoundAssetWriter::ASDCPState)
	, _asset (asset)
	, _frame_buffer_offset (0)
{
	/* Derived from ASDCP::Wav::SimpleWaveHeader::FillADesc */
	_state->desc.EditRate = ASDCP::Rational (_asset->edit_rate().numerator, _asset->edit_rate().denominator);
	_state->desc.AudioSamplingRate = ASDCP::Rational (_asset->sampling_rate(), 1);
	_state->desc.Locked = 0;
	_state->desc.ChannelCount = _asset->channels ();
	_state->desc.QuantizationBits = 24;
	_state->desc.BlockAlign = 3 * _asset->channels();
	_state->desc.AvgBps = _asset->sampling_rate() * _state->desc.BlockAlign;
	_state->desc.LinkedTrackID = 0;
	if (standard == INTEROP) {
		_state->desc.ChannelFormat = ASDCP::PCM::CF_NONE;
	} else {
		/* Just use WTF ("wild track format") for SMPTE for now; searches suggest that this
		   uses the same assignment as Interop.
		*/
		_state->desc.ChannelFormat = ASDCP::PCM::CF_CFG_4;
	}

	/* I'm fairly sure this is not necessary, as ContainerDuration is written
	   in ASDCP's WriteMXFFooter, but it stops a valgrind warning.
	*/
	_state->desc.ContainerDuration = 0;

	_state->frame_buffer.Capacity (ASDCP::PCM::CalcFrameBufferSize (_state->desc));
	_state->frame_buffer.Size (ASDCP::PCM::CalcFrameBufferSize (_state->desc));
	memset (_state->frame_buffer.Data(), 0, _state->frame_buffer.Capacity());

	_asset->fill_writer_info (&_state->writer_info, _asset->id(), standard);
}

void
SoundAssetWriter::write (float const * const * data, int frames)
{
	DCP_ASSERT (!_finalized);

	static float const clip = 1.0f - (1.0f / pow (2, 23));

	if (!_started) {
		Kumu::Result_t r = _state->mxf_writer.OpenWrite (_file.string().c_str(), _state->writer_info, _state->desc);
		if (ASDCP_FAILURE (r)) {
			boost::throw_exception (FileError ("could not open audio MXF for writing", _file.string(), r));
		}

		_asset->set_file (_file);
		_started = true;
	}

	int const ch = _asset->channels ();

	for (int i = 0; i < frames; ++i) {

		byte_t* out = _state->frame_buffer.Data() + _frame_buffer_offset;

		/* Write one sample per channel */
		for (int j = 0; j < ch; ++j) {
			/* Convert sample to 24-bit int, clipping if necessary. */
			float x = data[j][i];
			if (x > clip) {
				x = clip;
			} else if (x < -clip) {
				x = -clip;
			}
			int32_t const s = x * (1 << 23);
			*out++ = (s & 0xff);
			*out++ = (s & 0xff00) >> 8;
			*out++ = (s & 0xff0000) >> 16;
		}
		_frame_buffer_offset += 3 * ch;

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
	ASDCP::Result_t const r = _state->mxf_writer.WriteFrame (_state->frame_buffer, _encryption_context->encryption(), _encryption_context->hmac());
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

	_asset->_intrinsic_duration = _frames_written;
	return AssetWriter::finalize ();
}
