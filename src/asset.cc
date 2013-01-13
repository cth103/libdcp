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

/** @file  src/asset.cc
 *  @brief Parent class for assets of DCPs.
 */

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include "AS_DCP.h"
#include "KM_util.h"
#include "asset.h"
#include "util.h"
#include "metadata.h"

using namespace std;
using namespace boost;
using namespace libdcp;

Asset::Asset (string directory, string file_name)
	: _directory (directory)
	, _file_name (file_name)
	, _uuid (make_uuid ())
{
	if (_file_name.empty ()) {
		_file_name = _uuid + ".xml";
	}
}

void
Asset::write_to_pkl (ostream& s) const
{
	s << "    <Asset>\n"
	  << "      <Id>urn:uuid:" << _uuid << "</Id>\n"
	  << "      <AnnotationText>" << _file_name << "</AnnotationText>\n"
	  << "      <Hash>" << digest() << "</Hash>\n"
	  << "      <Size>" << filesystem::file_size(path()) << "</Size>\n"
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
	  << "          <Path>" << _file_name << "</Path>\n"
	  << "          <VolumeIndex>1</VolumeIndex>\n"
	  << "          <Offset>0</Offset>\n"
	  << "          <Length>" << filesystem::file_size(path()) << "</Length>\n"
	  << "        </Chunk>\n"
	  << "      </ChunkList>\n"
	  << "    </Asset>\n";
}

filesystem::path
Asset::path () const
{
	filesystem::path p;
	p /= _directory;
	p /= _file_name;
	return p;
}

string
Asset::digest () const
{
	if (_digest.empty ()) {
		_digest = make_digest (path().string());
	}

	return _digest;
}
