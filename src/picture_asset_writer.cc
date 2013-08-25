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
#include "picture_asset_writer.h"
#include "exceptions.h"
#include "picture_asset.h"

using std::istream;
using std::ostream;
using std::string;
using boost::shared_ptr;
using namespace libdcp;

FrameInfo::FrameInfo (istream& s)
	: offset (0)
	, size (0)
{
	s >> offset >> size;

	if (!s.good ()) {
		/* Make sure we zero these if something bad happened, otherwise
		   the caller might try to alloc lots of RAM.
		*/
		offset = size = 0;
	}

	s >> hash;
}

void
FrameInfo::write (ostream& s)
{
	s << offset << " " << size << " " << hash;
}


PictureAssetWriter::PictureAssetWriter (PictureAsset* asset, bool overwrite, bool interop, MXFMetadata const & metadata)
	: _asset (asset)
	, _frames_written (0)
	, _started (false)
	, _finalized (false)
	, _overwrite (overwrite)
	, _interop (interop)
	, _metadata (metadata)
{
	
}

struct ASDCPStateBase
{
	ASDCPStateBase ()
		: frame_buffer (4 * Kumu::Megabyte)
	{}
	
	ASDCP::JP2K::CodestreamParser j2k_parser;
	ASDCP::JP2K::FrameBuffer frame_buffer;
	ASDCP::WriterInfo writer_info;
	ASDCP::JP2K::PictureDescriptor picture_descriptor;
};

struct MonoPictureAssetWriter::ASDCPState : public ASDCPStateBase
{
	ASDCP::JP2K::MXFWriter mxf_writer;
};

struct StereoPictureAssetWriter::ASDCPState : public ASDCPStateBase
{
	ASDCP::JP2K::MXFSWriter mxf_writer;
};

/** @param a Asset to write to.  `a' must not be deleted while
 *  this writer class still exists, or bad things will happen.
 */
MonoPictureAssetWriter::MonoPictureAssetWriter (PictureAsset* asset, bool overwrite, bool interop, MXFMetadata const & metadata)
	: PictureAssetWriter (asset, overwrite, interop, metadata)
	, _state (new MonoPictureAssetWriter::ASDCPState)
{

}

StereoPictureAssetWriter::StereoPictureAssetWriter (PictureAsset* asset, bool overwrite, bool interop, MXFMetadata const & metadata)
	: PictureAssetWriter (asset, overwrite, interop, metadata)
	, _state (new StereoPictureAssetWriter::ASDCPState)
	, _next_eye (EYE_LEFT)
{

}

template <class P, class Q>
void libdcp::start (PictureAssetWriter* writer, shared_ptr<P> state, Q* asset, uint8_t* data, int size)
{
	if (ASDCP_FAILURE (state->j2k_parser.OpenReadFrame (data, size, state->frame_buffer))) {
		boost::throw_exception (MiscError ("could not parse J2K frame"));
	}

	state->j2k_parser.FillPictureDescriptor (state->picture_descriptor);
	state->picture_descriptor.EditRate = ASDCP::Rational (asset->edit_rate(), 1);
	
	asset->fill_writer_info (&state->writer_info, asset->uuid(), writer->_interop, writer->_metadata);
	
	if (ASDCP_FAILURE (state->mxf_writer.OpenWrite (
				   asset->path().string().c_str(),
				   state->writer_info,
				   state->picture_descriptor,
				   16384,
				   writer->_overwrite)
		    )) {
		
		boost::throw_exception (MXFFileError ("could not open MXF file for writing", asset->path().string()));
	}

	writer->_started = true;
}

void
MonoPictureAssetWriter::start (uint8_t* data, int size)
{
	libdcp::start (this, _state, _asset, data, size);
}

void
StereoPictureAssetWriter::start (uint8_t* data, int size)
{
	libdcp::start (this, _state, _asset, data, size);
}

FrameInfo
MonoPictureAssetWriter::write (uint8_t* data, int size)
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
	if (ASDCP_FAILURE (_state->mxf_writer.WriteFrame (_state->frame_buffer, 0, 0, &hash))) {
		boost::throw_exception (MXFFileError ("error in writing video MXF", _asset->path().string()));
	}

	++_frames_written;
	return FrameInfo (before_offset, _state->mxf_writer.Tell() - before_offset, hash);
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
	if (ASDCP_FAILURE (
		    _state->mxf_writer.WriteFrame (
			    _state->frame_buffer,
			    _next_eye == EYE_LEFT ? ASDCP::JP2K::SP_LEFT : ASDCP::JP2K::SP_RIGHT,
			    0,
			    0,
			    &hash)
		    )) {
		
		boost::throw_exception (MXFFileError ("error in writing video MXF", _asset->path().string()));
	}

	_next_eye = _next_eye == EYE_LEFT ? EYE_RIGHT : EYE_LEFT;

	return FrameInfo (before_offset, _state->mxf_writer.Tell() - before_offset, hash);
}

void
MonoPictureAssetWriter::fake_write (int size)
{
	assert (_started);
	assert (!_finalized);

	if (ASDCP_FAILURE (_state->mxf_writer.FakeWriteFrame (size))) {
		boost::throw_exception (MXFFileError ("error in writing video MXF", _asset->path().string()));
	}

	++_frames_written;
}

void
StereoPictureAssetWriter::fake_write (int size)
{
	assert (_started);
	assert (!_finalized);

	if (ASDCP_FAILURE (_state->mxf_writer.FakeWriteFrame (size))) {
		boost::throw_exception (MXFFileError ("error in writing video MXF", _asset->path().string()));
	}

	_next_eye = _next_eye == EYE_LEFT ? EYE_RIGHT : EYE_LEFT;
	++_frames_written;
}

void
MonoPictureAssetWriter::finalize ()
{
	assert (!_finalized);
	
	if (ASDCP_FAILURE (_state->mxf_writer.Finalize())) {
		boost::throw_exception (MXFFileError ("error in finalizing video MXF", _asset->path().string()));
	}

	_finalized = true;
	_asset->set_intrinsic_duration (_frames_written);
	_asset->set_duration (_frames_written);
}

void
StereoPictureAssetWriter::finalize ()
{
	assert (!_finalized);
	
	if (ASDCP_FAILURE (_state->mxf_writer.Finalize())) {
		boost::throw_exception (MXFFileError ("error in finalizing video MXF", _asset->path().string()));
	}

	_finalized = true;
	_asset->set_intrinsic_duration (_frames_written / 2);
	_asset->set_duration (_frames_written / 2);
}
