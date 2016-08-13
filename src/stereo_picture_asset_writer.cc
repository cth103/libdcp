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

#include "stereo_picture_asset_writer.h"
#include "exceptions.h"
#include "dcp_assert.h"
#include "picture_asset.h"
#include "encryption_context.h"
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_fileio.h>

#include "picture_asset_writer_common.cc"

using std::string;
using boost::shared_ptr;
using namespace dcp;

struct StereoPictureAssetWriter::ASDCPState : public ASDCPStateBase
{
	ASDCP::JP2K::MXFSWriter mxf_writer;
};

StereoPictureAssetWriter::StereoPictureAssetWriter (PictureAsset* mxf, boost::filesystem::path file, Standard standard, bool overwrite)
	: PictureAssetWriter (mxf, file, standard, overwrite)
	, _state (new StereoPictureAssetWriter::ASDCPState)
	, _next_eye (EYE_LEFT)
{

}

void
StereoPictureAssetWriter::start (uint8_t* data, int size)
{
	dcp::start (this, _state, _standard, _picture_asset, data, size);
	_picture_asset->set_frame_rate (Fraction (_picture_asset->edit_rate().numerator * 2, _picture_asset->edit_rate().denominator));
}

/** Write a frame for one eye.  Frames must be written left, then right, then left etc.
 *  @param data JPEG2000 data.
 *  @param size Size of data.
 */
FrameInfo
StereoPictureAssetWriter::write (uint8_t* data, int size)
{
	DCP_ASSERT (!_finalized);

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
		_encryption_context->encryption(),
		_encryption_context->hmac(),
		&hash
		);

	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("error in writing video MXF", _file.string(), r));
	}

	_next_eye = _next_eye == EYE_LEFT ? EYE_RIGHT : EYE_LEFT;

	if (_next_eye == EYE_LEFT) {
		++_frames_written;
	}

	return FrameInfo (before_offset, _state->mxf_writer.Tell() - before_offset, hash);
}

void
StereoPictureAssetWriter::fake_write (int size)
{
	DCP_ASSERT (_started);
	DCP_ASSERT (!_finalized);

	Kumu::Result_t r = _state->mxf_writer.FakeWriteFrame (size, _next_eye == EYE_LEFT ? ASDCP::JP2K::SP_LEFT : ASDCP::JP2K::SP_RIGHT);
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("error in writing video MXF", _file.string(), r));
	}

	_next_eye = _next_eye == EYE_LEFT ? EYE_RIGHT : EYE_LEFT;
	if (_next_eye == EYE_LEFT) {
		++_frames_written;
	}
}

bool
StereoPictureAssetWriter::finalize ()
{
	if (_started) {
		Kumu::Result_t r = _state->mxf_writer.Finalize();
		if (ASDCP_FAILURE (r)) {
			boost::throw_exception (MXFFileError ("error in finalizing video MXF", _file.string(), r));
		}
	}

	_picture_asset->_intrinsic_duration = _frames_written;
	return PictureAssetWriter::finalize ();
}
