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
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <libxml++/nodes/element.h>
#include "AS_DCP.h"
#include "KM_util.h"
#include "asset.h"
#include "util.h"
#include "metadata.h"

using namespace std;
using namespace boost;
using namespace libdcp;

Asset::Asset (boost::filesystem::path directory, string file_name, int edit_rate, int intrinsic_duration)
	: _directory (directory)
	, _file_name (file_name)
	, _uuid (make_uuid ())
	, _edit_rate (edit_rate)
	, _entry_point (0)
	, _intrinsic_duration (intrinsic_duration)
	, _duration (intrinsic_duration)
{
	if (_file_name.empty ()) {
		_file_name = _uuid + ".xml";
	}
}

void
Asset::write_to_pkl (xmlpp::Node* node) const
{
	xmlpp::Node* asset = node->add_child ("Asset");
	asset->add_child("Id")->add_child_text ("urn:uuid:" + _uuid);
	asset->add_child("AnnotationText")->add_child_text (_file_name);
	asset->add_child("Hash")->add_child_text (digest ());
	asset->add_child("Size")->add_child_text (lexical_cast<string> (filesystem::file_size(path())));
	asset->add_child("Type")->add_child_text ("application/mxf");
}

void
Asset::write_to_assetmap (xmlpp::Node* node) const
{
	xmlpp::Node* asset = node->add_child ("Asset");
	asset->add_child("Id")->add_child_text ("urn:uuid:" + _uuid);
	xmlpp::Node* chunk_list = asset->add_child ("ChunkList");
	xmlpp::Node* chunk = chunk_list->add_child ("Chunk");
	chunk->add_child("Path")->add_child_text (_file_name);
	chunk->add_child("VolumeIndex")->add_child_text ("1");
	chunk->add_child("Offset")->add_child_text ("0");
	chunk->add_child("Length")->add_child_text (lexical_cast<string> (filesystem::file_size(path())));
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
		_digest = make_digest (path().string(), 0);
	}

	return _digest;
}

void
Asset::compute_digest (boost::function<void (float)> progress)
{
	if (!_digest.empty ()) {
		return;
	}

	_digest = make_digest (path().string(), &progress);
}

bool
Asset::equals (shared_ptr<const Asset> other, EqualityOptions, boost::function<void (NoteType, string)> note) const
{
	if (_edit_rate != other->_edit_rate) {
		note (ERROR, "asset edit rates differ");
		return false;
	}
	
	if (_intrinsic_duration != other->_intrinsic_duration) {
		note (ERROR, "asset intrinsic durations differ");
	}

	if (_duration != other->_duration) {
		note (ERROR, "asset durations differ");
	}

	return true;
}
