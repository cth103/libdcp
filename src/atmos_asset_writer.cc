/*
    Copyright (C) 2016-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/atmos_asset_writer.cc
 *  @brief AtmosAssetWriter class
 */


#include "atmos_asset_writer.h"
#include "atmos_asset.h"
#include "exceptions.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include "crypto_context.h"
#include <asdcp/AS_DCP.h>


using std::min;
using std::max;
using std::shared_ptr;
using namespace dcp;


struct AtmosAssetWriter::ASDCPState
{
	ASDCP::ATMOS::MXFWriter mxf_writer;
	ASDCP::DCData::FrameBuffer frame_buffer;
	ASDCP::WriterInfo writer_info;
	ASDCP::ATMOS::AtmosDescriptor desc;
};


AtmosAssetWriter::AtmosAssetWriter (AtmosAsset* asset, boost::filesystem::path file)
	: AssetWriter (asset, file)
	, _state (new AtmosAssetWriter::ASDCPState)
	, _asset (asset)
{
	_state->desc.EditRate = ASDCP::Rational (_asset->edit_rate().numerator, _asset->edit_rate().denominator);
	_state->desc.FirstFrame = _asset->first_frame ();
	_state->desc.MaxChannelCount = _asset->max_channel_count ();
	_state->desc.MaxObjectCount = _asset->max_object_count ();

	unsigned int c;
	Kumu::hex2bin (_asset->atmos_id().c_str(), _state->desc.AtmosID, ASDCP::UUIDlen, &c);
	DCP_ASSERT (c == ASDCP::UUIDlen);

	_state->desc.AtmosVersion = _asset->atmos_version ();

	_asset->fill_writer_info (&_state->writer_info, _asset->id());
}


void
AtmosAssetWriter::write (shared_ptr<const AtmosFrame> frame)
{
	write (frame->data(), frame->size());
}


void
AtmosAssetWriter::write (uint8_t const * data, int size)
{
	DCP_ASSERT (!_finalized);

	if (!_started) {
		auto r = _state->mxf_writer.OpenWrite (_file.string().c_str(), _state->writer_info, _state->desc);
		if (ASDCP_FAILURE(r)) {
			boost::throw_exception (FileError ("could not open atmos MXF for writing", _file.string(), r));
		}

		_asset->set_file (_file);
		_started = true;
	}

	_state->frame_buffer.Capacity (size);
	_state->frame_buffer.Size (size);
	memcpy (_state->frame_buffer.Data(), data, size);

	auto const r = _state->mxf_writer.WriteFrame (_state->frame_buffer, _crypto_context->context(), _crypto_context->hmac());
	if (ASDCP_FAILURE(r)) {
		boost::throw_exception (MiscError(String::compose("could not write atmos MXF frame (%1)", static_cast<int>(r))));
	}

	++_frames_written;
}


bool
AtmosAssetWriter::finalize ()
{
	if (_started && ASDCP_FAILURE(_state->mxf_writer.Finalize())) {
		boost::throw_exception (MiscError("could not finalise atmos MXF"));
	}

	_asset->_intrinsic_duration = _frames_written;
	return AssetWriter::finalize ();
}
