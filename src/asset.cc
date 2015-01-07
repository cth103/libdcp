/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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
 *  @brief Asset class.
 */

#include "raw_convert.h"
#include "asset.h"
#include "util.h"
#include "exceptions.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include <libxml++/libxml++.h>

using std::string;
using boost::function;
using boost::optional;
using namespace dcp;

/** Create an Asset with a randomly-generated ID */
Asset::Asset ()
{

}

/** Create an Asset from a given file.
 *  @param file File name.
 */
Asset::Asset (boost::filesystem::path file)
	: _file (file)
{

}

void
Asset::write_to_pkl (xmlpp::Node* node, Standard standard) const
{
	DCP_ASSERT (!_file.empty ());
	
	xmlpp::Node* asset = node->add_child ("Asset");
	asset->add_child("Id")->add_child_text ("urn:uuid:" + _id);
	asset->add_child("AnnotationText")->add_child_text (_id);
	asset->add_child("Hash")->add_child_text (hash ());
	asset->add_child("Size")->add_child_text (raw_convert<string> (boost::filesystem::file_size (_file)));
	asset->add_child("Type")->add_child_text (pkl_type (standard));
}

void
Asset::write_to_assetmap (xmlpp::Node* node, boost::filesystem::path root) const
{
	DCP_ASSERT (!_file.empty ());

	xmlpp::Node* asset = node->add_child ("Asset");
	asset->add_child("Id")->add_child_text ("urn:uuid:" + _id);
	xmlpp::Node* chunk_list = asset->add_child ("ChunkList");
	xmlpp::Node* chunk = chunk_list->add_child ("Chunk");
	optional<boost::filesystem::path> path = relative_to_root (root, _file);
	if (!path) {
		throw MiscError (String::compose ("Asset %1 is not within the directory %2", _file, root));
	}
	chunk->add_child("Path")->add_child_text (path.get().string ());
	chunk->add_child("VolumeIndex")->add_child_text ("1");
	chunk->add_child("Offset")->add_child_text ("0");
	chunk->add_child("Length")->add_child_text (raw_convert<string> (boost::filesystem::file_size (_file)));
}

string
Asset::hash (function<void (float)> progress) const
{
	DCP_ASSERT (!_file.empty ());
		
	if (_hash.empty ()) {
		_hash = make_digest (_file, progress);
	}

	return _hash;
}

bool
Asset::equals (boost::shared_ptr<const Asset> other, EqualityOptions, NoteHandler note) const
{
	if (_hash != other->_hash) {
		note (DCP_ERROR, "Asset: hashes differ");
		return false;
	}

	return true;
}

void
Asset::set_file (boost::filesystem::path file) const
{
	_file = boost::filesystem::absolute (file);
	_hash.clear ();
}
	
