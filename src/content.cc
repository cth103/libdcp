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
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <libxml++/nodes/element.h>
#include "AS_DCP.h"
#include "KM_util.h"
#include "content.h"
#include "util.h"
#include "metadata.h"

using namespace std;
using namespace boost;
using namespace dcp;

Content::Content (boost::filesystem::path file)
	: _file (file)
	, _edit_rate (0)
	, _intrinsic_duration (0)
{

}

Content::Content (int edit_rate)
	: _edit_rate (edit_rate)
	, _intrinsic_duration (0)
{

}

void
Content::write_to_pkl (xmlpp::Node* node) const
{
	xmlpp::Node* asset = node->add_child ("Asset");
	asset->add_child("Id")->add_child_text ("urn:uuid:" + _id);
	asset->add_child("AnnotationText")->add_child_text (_id);
//XXX	asset->add_child("Hash")->add_child_text (digest ());
	asset->add_child("Size")->add_child_text (lexical_cast<string> (filesystem::file_size (_file)));
	asset->add_child("Type")->add_child_text ("application/mxf");
}

void
Content::write_to_assetmap (xmlpp::Node* node) const
{
	xmlpp::Node* asset = node->add_child ("Asset");
	asset->add_child("Id")->add_child_text ("urn:uuid:" + _id);
	xmlpp::Node* chunk_list = asset->add_child ("ChunkList");
	xmlpp::Node* chunk = chunk_list->add_child ("Chunk");
	chunk->add_child("Path")->add_child_text (_file.string ());
	chunk->add_child("VolumeIndex")->add_child_text ("1");
	chunk->add_child("Offset")->add_child_text ("0");
	chunk->add_child("Length")->add_child_text (lexical_cast<string> (filesystem::file_size (_file)));
}

bool
Content::equals (shared_ptr<const Content> other, EqualityOptions, boost::function<void (NoteType, string)> note) const
{
	// if (_edit_rate != other->_edit_rate) {
	// 	note (ERROR, "asset edit rates differ");
	// 	return false;
	// }
	
	// if (_intrinsic_duration != other->_intrinsic_duration) {
	// 	note (ERROR, "asset intrinsic durations differ");
	// }

	// if (_duration != other->_duration) {
	// 	note (ERROR, "asset durations differ");
	// }

	// return true;
}
