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


/** @file  src/mono_picture_asset_writer.cc
 *  @brief MonoJ2KPictureAssetWriter class
 */


#include "crypto_context.h"
#include "dcp_assert.h"
#include "exceptions.h"
#include "mono_j2k_picture_asset_writer.h"
#include "j2k_picture_asset.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_fileio.h>
LIBDCP_ENABLE_WARNINGS


#include "j2k_picture_asset_writer_common.cc"


using std::string;
using std::shared_ptr;
using namespace dcp;


struct MonoJ2KPictureAssetWriter::ASDCPState : public ASDCPJ2KStateBase
{
	ASDCP::JP2K::MXFWriter mxf_writer;
};


/** @param a Asset to write to.  `a' must not be deleted while
 *  this writer class still exists, or bad things will happen.
 */
MonoJ2KPictureAssetWriter::MonoJ2KPictureAssetWriter (J2KPictureAsset* asset, boost::filesystem::path file, bool overwrite)
	: J2KPictureAssetWriter (asset, file, overwrite)
	, _state (new MonoJ2KPictureAssetWriter::ASDCPState)
{

}


MonoJ2KPictureAssetWriter::~MonoJ2KPictureAssetWriter()
{
	try {
		/* Last-resort finalization to close the file, at least */
		if (!_finalized) {
			_state->mxf_writer.Finalize();
		}
	} catch (...) {}
}


void
MonoJ2KPictureAssetWriter::start (uint8_t const * data, int size)
{
	dcp::start (this, _state, _picture_asset, data, size);
	_picture_asset->set_frame_rate (_picture_asset->edit_rate());
}


J2KFrameInfo
MonoJ2KPictureAssetWriter::write (uint8_t const * data, int size)
{
	DCP_ASSERT (!_finalized);

	if (!_started) {
		start (data, size);
	}

 	if (ASDCP_FAILURE (_state->j2k_parser.OpenReadFrame(data, size, _state->frame_buffer))) {
 		boost::throw_exception (MiscError ("could not parse J2K frame"));
 	}

	_state->frame_buffer.PlaintextOffset(0);

	uint64_t const before_offset = _state->mxf_writer.Tell ();

	string hash;
	auto const r = _state->mxf_writer.WriteFrame (_state->frame_buffer, _crypto_context->context(), _crypto_context->hmac(), &hash);
	if (ASDCP_FAILURE(r)) {
		boost::throw_exception (MXFFileError ("error in writing video MXF", _file.string(), r));
	}

	++_frames_written;
	return J2KFrameInfo(before_offset, _state->mxf_writer.Tell() - before_offset, hash);
}


void
MonoJ2KPictureAssetWriter::fake_write(J2KFrameInfo const& info)
{
	DCP_ASSERT (_started);
	DCP_ASSERT (!_finalized);

	auto r = _state->mxf_writer.FakeWriteFrame(info.size);
	if (ASDCP_FAILURE(r)) {
		boost::throw_exception (MXFFileError("error in writing video MXF", _file.string(), r));
	}

	++_frames_written;
}


bool
MonoJ2KPictureAssetWriter::finalize ()
{
	if (_started) {
		auto r = _state->mxf_writer.Finalize();
		if (ASDCP_FAILURE(r)) {
			boost::throw_exception (MXFFileError("error in finalizing video MXF", _file.string(), r));
		}
	}

	_picture_asset->_intrinsic_duration = _frames_written;
	return J2KPictureAssetWriter::finalize ();
}
