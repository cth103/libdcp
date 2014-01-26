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

using boost::shared_ptr;

struct ASDCPStateBase
{
	ASDCPStateBase ()
		: frame_buffer (4 * Kumu::Megabyte)
	{}
	
	ASDCP::JP2K::CodestreamParser j2k_parser;
	ASDCP::JP2K::FrameBuffer frame_buffer;
	ASDCP::WriterInfo writer_info;
	ASDCP::JP2K::PictureDescriptor picture_descriptor;
	ASDCP::AESEncContext* encryption_context;
};

template <class P, class Q>
void dcp::start (PictureMXFWriter* writer, shared_ptr<P> state, Standard standard, Q* mxf, uint8_t* data, int size)
{
	mxf->set_file (writer->_file);
	
	if (ASDCP_FAILURE (state->j2k_parser.OpenReadFrame (data, size, state->frame_buffer))) {
		boost::throw_exception (MiscError ("could not parse J2K frame"));
	}

	state->j2k_parser.FillPictureDescriptor (state->picture_descriptor);
	state->picture_descriptor.EditRate = ASDCP::Rational (mxf->edit_rate().numerator, mxf->edit_rate().denominator);
	
	mxf->fill_writer_info (&state->writer_info, standard);
	
	Kumu::Result_t r = state->mxf_writer.OpenWrite (
		mxf->file().string().c_str(),
		state->writer_info,
		state->picture_descriptor,
		16384,
		writer->_overwrite
		);

	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for writing", mxf->file().string(), r));
	}

	writer->_started = true;
}
