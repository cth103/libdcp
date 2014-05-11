/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#include "mono_picture_asset.h"
#include "mono_picture_asset_writer.h"
#include "AS_DCP.h"
#include "KM_fileio.h"
#include "exceptions.h"
#include "mono_picture_frame.h"

using std::string;
using std::vector;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using boost::lexical_cast;
using namespace libdcp;

MonoPictureAsset::MonoPictureAsset (boost::filesystem::path directory, boost::filesystem::path mxf_name)
	: PictureAsset (directory, mxf_name)
{

}

void
MonoPictureAsset::create (vector<boost::filesystem::path> const & files)
{
	create (boost::bind (&MonoPictureAsset::path_from_list, this, _1, files));
}

void
MonoPictureAsset::create (boost::function<boost::filesystem::path (int)> get_path)
{
	ASDCP::JP2K::CodestreamParser j2k_parser;
	ASDCP::JP2K::FrameBuffer frame_buffer (4 * Kumu::Megabyte);
	Kumu::Result_t r = j2k_parser.OpenReadFrame (get_path(0).string().c_str(), frame_buffer);
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (FileError ("could not open JPEG2000 file for reading", get_path(0), r));
	}
	
	ASDCP::JP2K::PictureDescriptor picture_desc;
	j2k_parser.FillPictureDescriptor (picture_desc);
	picture_desc.EditRate = ASDCP::Rational (_edit_rate, 1);
	
	ASDCP::WriterInfo writer_info;
	fill_writer_info (&writer_info);
	
	ASDCP::JP2K::MXFWriter mxf_writer;
	r = mxf_writer.OpenWrite (path().string().c_str(), writer_info, picture_desc, 16384, false);
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for writing", path().string(), r));
	}

	for (int i = 0; i < _intrinsic_duration; ++i) {

		boost::filesystem::path const path = get_path (i);

		Kumu::Result_t r = j2k_parser.OpenReadFrame (path.string().c_str(), frame_buffer);
		if (ASDCP_FAILURE (r)) {
			boost::throw_exception (FileError ("could not open JPEG2000 file for reading", path, r));
		}

		r = mxf_writer.WriteFrame (frame_buffer, _encryption_context, 0);
		if (ASDCP_FAILURE (r)) {
			boost::throw_exception (MXFFileError ("error in writing video MXF", this->path().string(), r));
		}

		if (_progress) {
			(*_progress) (0.5 * float (i) / _intrinsic_duration);
		}
	}
	
	r = mxf_writer.Finalize();
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("error in finalising video MXF", path().string(), r));
	}
}

void
MonoPictureAsset::read ()
{
	ASDCP::JP2K::MXFReader reader;
	Kumu::Result_t r = reader.OpenRead (path().string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", path().string(), r));
	}
	
	ASDCP::JP2K::PictureDescriptor desc;
	if (ASDCP_FAILURE (reader.FillPictureDescriptor (desc))) {
		boost::throw_exception (DCPReadError ("could not read video MXF information"));
	}

	_size.width = desc.StoredWidth;
	_size.height = desc.StoredHeight;
	_edit_rate = desc.EditRate.Numerator;
	assert (desc.EditRate.Denominator == 1);
	_intrinsic_duration = desc.ContainerDuration;
}

boost::filesystem::path
MonoPictureAsset::path_from_list (int f, vector<boost::filesystem::path> const & files) const
{
	return files[f];
}

shared_ptr<const MonoPictureFrame>
MonoPictureAsset::get_frame (int n) const
{
	return shared_ptr<const MonoPictureFrame> (new MonoPictureFrame (path(), n, _decryption_context));
}

bool
MonoPictureAsset::equals (shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (!MXFAsset::equals (other, opt, note)) {
		return false;
	}

	ASDCP::JP2K::MXFReader reader_A;
	Kumu::Result_t r = reader_A.OpenRead (path().string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", path().string(), r));
	}
	
	ASDCP::JP2K::MXFReader reader_B;
	r = reader_B.OpenRead (other->path().string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", path().string(), r));
	}
	
	ASDCP::JP2K::PictureDescriptor desc_A;
	if (ASDCP_FAILURE (reader_A.FillPictureDescriptor (desc_A))) {
		boost::throw_exception (DCPReadError ("could not read video MXF information"));
	}
	ASDCP::JP2K::PictureDescriptor desc_B;
	if (ASDCP_FAILURE (reader_B.FillPictureDescriptor (desc_B))) {
		boost::throw_exception (DCPReadError ("could not read video MXF information"));
	}
	
	if (!descriptor_equals (desc_A, desc_B, note)) {
		return false;
	}

	shared_ptr<const MonoPictureAsset> other_picture = dynamic_pointer_cast<const MonoPictureAsset> (other);
	assert (other_picture);

	for (int i = 0; i < _intrinsic_duration; ++i) {
		if (i >= other_picture->intrinsic_duration()) {
			return false;
		}
		
		note (PROGRESS, "Comparing video frame " + lexical_cast<string> (i) + " of " + lexical_cast<string> (_intrinsic_duration));
		shared_ptr<const MonoPictureFrame> frame_A = get_frame (i);
		shared_ptr<const MonoPictureFrame> frame_B = other_picture->get_frame (i);
		
		if (!frame_buffer_equals (
			    i, opt, note,
			    frame_A->j2k_data(), frame_A->j2k_size(),
			    frame_B->j2k_data(), frame_B->j2k_size()
			    )) {
			return false;
		}
	}

	return true;
}

shared_ptr<PictureAssetWriter>
MonoPictureAsset::start_write (bool overwrite)
{
	/* XXX: can't we use shared_ptr here? */
	return shared_ptr<MonoPictureAssetWriter> (new MonoPictureAssetWriter (this, overwrite));
}

string
MonoPictureAsset::cpl_node_name () const
{
	return "MainPicture";
}

int
MonoPictureAsset::edit_rate_factor () const
{
	return 1;
}
