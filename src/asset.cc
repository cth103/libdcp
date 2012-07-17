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

#include <iostream>
#include <boost/filesystem.hpp>
#include "AS_DCP.h"
#include "KM_util.h"
#include "asset.h"
#include "util.h"
#include "tags.h"

using namespace std;
using namespace boost;
using namespace libdcp;

/** Construct an Asset.
 *  @param p Pathname of MXF file.
 *  @param fps Frames per second.
 *  @param len Length in frames.
 */

Asset::Asset (string p, int fps, int len)
	: _mxf_path (p)
	, _fps (fps)
	, _length (len)
	, _uuid (make_uuid ())
{
	
}

void
Asset::write_to_pkl (ostream& s) const
{
	s << "    <Asset>\n"
	  << "      <Id>urn:uuid:" << _uuid << "</Id>\n"
	  << "      <AnnotationText>" << filesystem::path(_mxf_path).filename() << "</AnnotationText>\n"
	  << "      <Hash>" << _digest << "</Hash>\n"
	  << "      <Size>" << filesystem::file_size(_mxf_path) << "</Size>\n"
	  << "      <Type>application/mxf</Type>\n"
	  << "    </Asset>\n";
}

void
Asset::write_to_assetmap (ostream& s) const
{
	s << "    <Asset>\n"
	  << "      <Id>urn:uuid:" << _uuid << "</Id>\n"
	  << "      <ChunkList>\n"
	  << "        <Chunk>\n"
	  << "          <Path>" << filesystem::path(_mxf_path).filename() << "</Path>\n"
	  << "          <VolumeIndex>1</VolumeIndex>\n"
	  << "          <Offset>0</Offset>\n"
	  << "          <Length>" << filesystem::file_size(_mxf_path) << "</Length>\n"
	  << "        </Chunk>\n"
	  << "      </ChunkList>\n"
	  << "    </Asset>\n";
}

void
Asset::fill_writer_info (ASDCP::WriterInfo* writer_info) const
{
	writer_info->ProductVersion = Tags::instance()->product_version;
	writer_info->CompanyName = Tags::instance()->company_name;
	writer_info->ProductName = Tags::instance()->product_name.c_str();

	writer_info->LabelSetType = ASDCP::LS_MXF_SMPTE;
	Kumu::GenRandomUUID (writer_info->AssetUUID);
}
