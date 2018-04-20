/*
    Copyright (C) 2014-2018 Carl Hetherington <cth@carlh.net>

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

/** @file  src/asset.cc
 *  @brief Asset class.
 */

#include "raw_convert.h"
#include "asset.h"
#include "util.h"
#include "exceptions.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include "pkl.h"
#include <libxml++/libxml++.h>
#include <boost/algorithm/string.hpp>

using std::string;
using boost::function;
using boost::shared_ptr;
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

/** Create an Asset from a given file with a known ID.
 *  @param file File name.
 *  @param id ID.
 */
Asset::Asset (string id, boost::filesystem::path file)
	: Object (id)
	, _file (file)
{

}

void
Asset::add_to_pkl (shared_ptr<PKL> pkl, boost::filesystem::path root) const
{
	DCP_ASSERT (_file);

	optional<boost::filesystem::path> path = relative_to_root (
		boost::filesystem::canonical (root),
		boost::filesystem::canonical (_file.get())
		);

	if (!path) {
		/* The path of this asset is not within our DCP, so we assume it's an external
		   (referenced) one.
		*/
		return;
	}

	pkl->add_asset (_id, _id, hash(), boost::filesystem::file_size (_file.get()), pkl_type (pkl->standard()));
}

void
Asset::write_to_assetmap (xmlpp::Node* node, boost::filesystem::path root) const
{
	DCP_ASSERT (_file);

	optional<boost::filesystem::path> path = relative_to_root (
		boost::filesystem::canonical (root),
		boost::filesystem::canonical (_file.get())
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
	chunk->add_child("Length")->add_child_text (raw_convert<string> (boost::filesystem::file_size (_file.get())));
}

string
Asset::hash (function<void (float)> progress) const
{
	DCP_ASSERT (_file);

	if (!_hash) {
		_hash = make_digest (_file.get(), progress);
	}

	return _hash.get();
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
	_hash = optional<string> ();
}

void
Asset::set_hash (string hash)
{
	_hash = hash;
}
