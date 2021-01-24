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


/** @file  src/stereo_picture_asset.cc
 *  @brief StereoPictureAsset class
 */


#include "stereo_picture_asset.h"
#include "stereo_picture_frame.h"
#include "exceptions.h"
#include "stereo_picture_asset_writer.h"
#include "stereo_picture_asset_reader.h"
#include "dcp_assert.h"
#include <asdcp/AS_DCP.h>


using std::string;
using std::pair;
using std::make_pair;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using namespace dcp;


StereoPictureAsset::StereoPictureAsset (boost::filesystem::path file)
	: PictureAsset (file)
{
	ASDCP::JP2K::MXFSReader reader;
	auto r = reader.OpenRead (file.string().c_str());
	if (ASDCP_FAILURE(r)) {
		boost::throw_exception (MXFFileError("could not open MXF file for reading", file.string(), r));
	}

	ASDCP::JP2K::PictureDescriptor desc;
	if (ASDCP_FAILURE (reader.FillPictureDescriptor(desc))) {
		boost::throw_exception (ReadError("could not read video MXF information"));
	}

	read_picture_descriptor (desc);

	ASDCP::WriterInfo info;
	if (ASDCP_FAILURE (reader.FillWriterInfo(info))) {
		boost::throw_exception (ReadError("could not read video MXF information"));
	}

	_id = read_writer_info (info);
}


StereoPictureAsset::StereoPictureAsset (Fraction edit_rate, Standard standard)
	: PictureAsset (edit_rate, standard)
{

}


shared_ptr<PictureAssetWriter>
StereoPictureAsset::start_write (boost::filesystem::path file, bool overwrite)
{
	return shared_ptr<StereoPictureAssetWriter> (new StereoPictureAssetWriter(this, file, overwrite));
}


shared_ptr<StereoPictureAssetReader>
StereoPictureAsset::start_read () const
{
	return shared_ptr<StereoPictureAssetReader> (new StereoPictureAssetReader(this, key(), standard()));
}


bool
StereoPictureAsset::equals (shared_ptr<const Asset> other, EqualityOptions opt, NoteHandler note) const
{
	ASDCP::JP2K::MXFSReader reader_A;
	DCP_ASSERT (file());
	auto r = reader_A.OpenRead (file()->string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", file()->string(), r));
	}

	ASDCP::JP2K::MXFSReader reader_B;
	DCP_ASSERT (other->file());
	r = reader_B.OpenRead (other->file()->string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", other->file()->string(), r));
	}

	ASDCP::JP2K::PictureDescriptor desc_A;
	if (ASDCP_FAILURE (reader_A.FillPictureDescriptor (desc_A))) {
		boost::throw_exception (ReadError ("could not read video MXF information"));
	}
	ASDCP::JP2K::PictureDescriptor desc_B;
	if (ASDCP_FAILURE (reader_B.FillPictureDescriptor (desc_B))) {
		boost::throw_exception (ReadError ("could not read video MXF information"));
	}

	if (!descriptor_equals (desc_A, desc_B, note)) {
		return false;
	}

	auto other_picture = dynamic_pointer_cast<const StereoPictureAsset> (other);
	DCP_ASSERT (other_picture);

	auto reader = start_read ();
	auto other_reader = other_picture->start_read ();

	bool result = true;

	for (int i = 0; i < _intrinsic_duration; ++i) {
		shared_ptr<const StereoPictureFrame> frame_A;
		shared_ptr<const StereoPictureFrame> frame_B;
		try {
			frame_A = reader->get_frame (i);
			frame_B = other_reader->get_frame (i);
		} catch (ReadError& e) {
			/* If there was a problem reading the frame data we'll just assume
			   the two frames are not equal.
			*/
			note (NoteType::ERROR, e.what ());
			return false;
		}

		if (!frame_buffer_equals (
			    i, opt, note,
			    frame_A->left()->data(), frame_A->left()->size(),
			    frame_B->left()->data(), frame_B->left()->size()
			    )) {
			result = false;
			if (!opt.keep_going) {
				return result;
			}
		}

		if (!frame_buffer_equals (
			    i, opt, note,
			    frame_A->right()->data(), frame_A->right()->size(),
			    frame_B->right()->data(), frame_B->right()->size()
			    )) {
			result = false;
			if (!opt.keep_going) {
				return result;
			}
		}
	}

	return result;
}
