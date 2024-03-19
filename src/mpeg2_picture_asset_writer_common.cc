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


/** @file  src/mpeg2_picture_asset_writer_common.cc
 *  @brief Common parts of MPEG2PictureAssetWriter
 */


#include "filesystem.h"


using std::shared_ptr;


namespace dcp {


class MPEG2PictureAssetWriter;


struct ASDCPMPEG2StateBase
{
	ASDCP::MPEG2::Parser mpeg2_parser;
	ASDCP::WriterInfo writer_info;
	ASDCP::MPEG2::VideoDescriptor video_descriptor;
};


}


template <class P, class Q>
void dcp::start(MPEG2PictureAssetWriter* writer, shared_ptr<P> state, Q* asset, uint8_t const * data, int size)
{
	asset->set_file (writer->_file);

	if (ASDCP_FAILURE(state->mpeg2_parser.OpenRead(data, size))) {
		boost::throw_exception(MiscError("could not parse MPEG2 frame"));
	}

	state->mpeg2_parser.FillVideoDescriptor(state->video_descriptor);
	state->video_descriptor.EditRate = ASDCP::Rational(asset->edit_rate().numerator, asset->edit_rate().denominator);

	asset->set_size(Size(state->video_descriptor.StoredWidth, state->video_descriptor.StoredHeight));
	asset->set_screen_aspect_ratio(Fraction(state->video_descriptor.AspectRatio.Numerator, state->video_descriptor.AspectRatio.Denominator));

	asset->fill_writer_info(&state->writer_info, asset->id());

	auto r = state->mxf_writer.OpenWrite(
		dcp::filesystem::fix_long_path(*asset->file()).string().c_str(),
		state->writer_info,
		state->video_descriptor,
		16384,
		writer->_overwrite
		);

	if (ASDCP_FAILURE(r)) {
		boost::throw_exception(MXFFileError("could not open MXF file for writing", asset->file()->string(), r));
	}

	writer->_started = true;
}
