/*
    Copyright (C) 2012-2013 Carl Hetherington <cth@carlh.net>

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

#include "AS_DCP.h"
#include "KM_fileio.h"
#include "stereo_picture_asset_writer.h"
#include "exceptions.h"
#include "picture_asset.h"

#include "picture_asset_writer_common.cc"

using std::istream;
using std::ostream;
using std::string;
using boost::shared_ptr;
using namespace libdcp;

struct StereoPictureAssetWriter::ASDCPState : public ASDCPStateBase
{
	ASDCP::JP2K::MXFSWriter mxf_writer;
};

StereoPictureAssetWriter::StereoPictureAssetWriter (PictureAsset* asset, bool overwrite)
	: PictureAssetWriter (asset, overwrite)
	, _state (new StereoPictureAssetWriter::ASDCPState)
	, _next_eye (EYE_LEFT)
{
	_state->encryption_context = asset->encryption_context ();
}

void
StereoPictureAssetWriter::start (uint8_t* data, int size)
{
	libdcp::start (this, _state, _asset, data, size);
}

/** Write a frame for one eye.  Frames must be written left, then right, then left etc.
 *  @param data JPEG2000 data.
 *  @param size Size of data.
 */
FrameInfo
StereoPictureAssetWriter::write (uint8_t* data, int size)
{
	assert (!_finalized);

	if (!_started) {
		start (data, size);
	}

 	if (ASDCP_FAILURE (_state->j2k_parser.OpenReadFrame (data, size, _state->frame_buffer))) {
 		boost::throw_exception (MiscError ("could not parse J2K frame"));
 	}

	uint64_t const before_offset = _state->mxf_writer.Tell ();

	string hash;
	Kumu::Result_t r = _state->mxf_writer.WriteFrame (
		_state->frame_buffer,
		_next_eye == EYE_LEFT ? ASDCP::JP2K::SP_LEFT : ASDCP::JP2K::SP_RIGHT,
		_state->encryption_context,
		0,
		&hash
		);

	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("error in writing video MXF", _asset->path().string(), r));
	}

	_next_eye = _next_eye == EYE_LEFT ? EYE_RIGHT : EYE_LEFT;

	++_frames_written;
	return FrameInfo (before_offset, _state->mxf_writer.Tell() - before_offset, hash);
}

void
StereoPictureAssetWriter::fake_write (int size)
{
	assert (_started);
	assert (!_finalized);

	Kumu::Result_t r = _state->mxf_writer.FakeWriteFrame (size, _next_eye == EYE_LEFT ? ASDCP::JP2K::SP_LEFT : ASDCP::JP2K::SP_RIGHT);
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("error in writing video MXF", _asset->path().string(), r));
	}

	_next_eye = _next_eye == EYE_LEFT ? EYE_RIGHT : EYE_LEFT;
	++_frames_written;
}

void
StereoPictureAssetWriter::finalize ()
{
	assert (!_finalized);
	
	Kumu::Result_t r = _state->mxf_writer.Finalize();
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("error in finalizing video MXF", _asset->path().string(), r));
	}

	_finalized = true;
	_asset->set_intrinsic_duration (_frames_written / 2);
	_asset->set_duration (_frames_written / 2);
}
