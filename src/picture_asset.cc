/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

/** @file  src/picture_asset.h
 *  @brief An asset made up of JPEG2000 files
 */

#include <list>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <boost/filesystem.hpp>
#include "AS_DCP.h"
#include "KM_fileio.h"
#include "picture_asset.h"
#include "util.h"

using namespace std;
using namespace boost;
using namespace libdcp;

PictureAsset::PictureAsset (
	list<string> const & files,
	string mxf_path,
	sigc::signal1<void, float>* progress,
	int fps,
	int length,
	int width,
	int height)
	: Asset (mxf_path, progress, fps, length)
	, _width (width)
	, _height (height)
{
	ASDCP::JP2K::CodestreamParser j2k_parser;
	ASDCP::JP2K::FrameBuffer frame_buffer (4 * Kumu::Megabyte);
	if (ASDCP_FAILURE (j2k_parser.OpenReadFrame (files.front().c_str(), frame_buffer))) {
		stringstream s;
		s << "could not open " << files.front() << " for reading";
		throw runtime_error (s.str());
	}
	
	ASDCP::JP2K::PictureDescriptor picture_desc;
	j2k_parser.FillPictureDescriptor (picture_desc);
	picture_desc.EditRate = ASDCP::Rational (_fps, 1);
	
	ASDCP::WriterInfo writer_info;
	fill_writer_info (&writer_info);
	
	ASDCP::JP2K::MXFWriter mxf_writer;
	if (ASDCP_FAILURE (mxf_writer.OpenWrite (_mxf_path.c_str(), writer_info, picture_desc))) {
		throw runtime_error ("could not open MXF for writing");
	}

	int j = 0;
	for (list<string>::const_iterator i = files.begin(); i != files.end(); ++i) {
		if (ASDCP_FAILURE (j2k_parser.OpenReadFrame (i->c_str(), frame_buffer))) {
			stringstream s;
			s << "could not open " << *i << " for reading";
			throw runtime_error (s.str());
		}

		/* XXX: passing 0 to WriteFrame ok? */
		if (ASDCP_FAILURE (mxf_writer.WriteFrame (frame_buffer, 0, 0))) {
			throw runtime_error ("error in writing video MXF");
		}
		
		++j;
		(*_progress) (0.5 * float (j) / files.size ());
	}
	
	if (ASDCP_FAILURE (mxf_writer.Finalize())) {
		throw runtime_error ("error in finalising video MXF");
	}

	_digest = make_digest (_mxf_path, _progress);
}

void
PictureAsset::write_to_cpl (ostream& s) const
{
	s << "        <MainPicture>\n"
	  << "          <Id>urn:uuid:" << _uuid << "</Id>\n"
	  << "          <AnnotationText>" << filesystem::path(_mxf_path).filename() << "</AnnotationText>\n"
	  << "          <EditRate>" << _fps << " 1</EditRate>\n"
	  << "          <IntrinsicDuration>" << _length << "</IntrinsicDuration>\n"
	  << "          <EntryPoint>0</EntryPoint>\n"
	  << "          <Duration>" << _length << "</Duration>\n"
	  << "          <FrameRate>" << _fps << " 1</FrameRate>\n"
	  << "          <ScreenAspectRatio>" << _width << " " << _height << "</ScreenAspectRatio>\n"
	  << "        </MainPicture>\n";
}
