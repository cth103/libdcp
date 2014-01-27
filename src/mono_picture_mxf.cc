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

#include "mono_picture_mxf.h"
#include "mono_picture_mxf_writer.h"
#include "AS_DCP.h"
#include "KM_fileio.h"
#include "exceptions.h"
#include "mono_picture_frame.h"
#include "compose.hpp"

using std::string;
using std::vector;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using namespace dcp;

MonoPictureMXF::MonoPictureMXF (boost::filesystem::path file)
	: PictureMXF (file)
{
	ASDCP::JP2K::MXFReader reader;
	Kumu::Result_t r = reader.OpenRead (file.string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", file.string(), r));
	}
	
	ASDCP::JP2K::PictureDescriptor desc;
	if (ASDCP_FAILURE (reader.FillPictureDescriptor (desc))) {
		boost::throw_exception (DCPReadError ("could not read video MXF information"));
	}

	read_picture_descriptor (desc);
	
	ASDCP::WriterInfo info;
	if (ASDCP_FAILURE (reader.FillWriterInfo (info))) {
		boost::throw_exception (DCPReadError ("could not read video MXF information"));
	}

	read_writer_info (info);
}

MonoPictureMXF::MonoPictureMXF (Fraction edit_rate)
	: PictureMXF (edit_rate)
{
	
}

shared_ptr<const MonoPictureFrame>
MonoPictureMXF::get_frame (int n) const
{
	return shared_ptr<const MonoPictureFrame> (new MonoPictureFrame (_file, n, _decryption_context));
}

bool
MonoPictureMXF::equals (shared_ptr<const Content> other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (!MXF::equals (other, opt, note)) {
		return false;
	}

	ASDCP::JP2K::MXFReader reader_A;
	Kumu::Result_t r = reader_A.OpenRead (_file.string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", _file.string(), r));
	}
	
	ASDCP::JP2K::MXFReader reader_B;
	r = reader_B.OpenRead (other->file().string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", other->file().string(), r));
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

	shared_ptr<const MonoPictureMXF> other_picture = dynamic_pointer_cast<const MonoPictureMXF> (other);
	assert (other_picture);

	for (int i = 0; i < _intrinsic_duration; ++i) {
		if (i >= other_picture->intrinsic_duration()) {
			return false;
		}
		
		note (PROGRESS, String::compose ("Comparing video frame %1 of %2", i, _intrinsic_duration));
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

shared_ptr<PictureMXFWriter>
MonoPictureMXF::start_write (boost::filesystem::path file, Standard standard, bool overwrite)
{
	/* XXX: can't we use shared_ptr here? */
	return shared_ptr<MonoPictureMXFWriter> (new MonoPictureMXFWriter (this, file, standard, overwrite));
}

string
MonoPictureMXF::cpl_node_name () const
{
	return "MainPicture";
}
