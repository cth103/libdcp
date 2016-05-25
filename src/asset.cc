/*
    Copyright (C) 2014-2015 Carl Hetherington <cth@carlh.net>

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
#include <boost/algorithm/string.hpp>

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

Asset::Asset (string id, boost::filesystem::path file)
	: Object (id)
	, _file (file)
{

}

void
Asset::write_to_pkl (xmlpp::Node* node, boost::filesystem::path root, Standard standard) const
{
	DCP_ASSERT (!_file.empty ());

	optional<boost::filesystem::path> path = relative_to_root (
		boost::filesystem::canonical (root),
		boost::filesystem::canonical (_file)
		);

	if (!path) {
		/* The path of this asset is not within our DCP, so we assume it's an external
		   (referenced) one.
		*/
		return;
	}

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

	optional<boost::filesystem::path> path = relative_to_root (
		boost::filesystem::canonical (root),
		boost::filesystem::canonical (_file)
		);

	if (!path) {
		/* The path of this asset is not within our DCP, so we assume it's an external
		   (referenced) one.
		*/
		return;
	}

	xmlpp::Node* asset = node->add_child ("Asset");
	asset->add_child("Id")->add_child_text ("urn:uuid:" + _id);
	xmlpp::Node* chunk_list = asset->add_child ("ChunkList");
	xmlpp::Node* chunk = chunk_list->add_child ("Chunk");

	chunk->add_child("Path")->add_child_text (path.get().generic_string());
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

/** Set the file that holds this asset on disk.  Calling this function
 *  clears this object's store of its hash, so you should call ::hash
 *  after this.
 *
 *  @param file New file's path.
 */
void
Asset::set_file (boost::filesystem::path file) const
{
	_file = boost::filesystem::absolute (file);
	_hash.clear ();
}
