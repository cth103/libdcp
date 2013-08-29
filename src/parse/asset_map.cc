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

/** @file  src/asset_map.cc
 *  @brief Classes used to parse a AssetMap.
 */

#include <boost/algorithm/string.hpp>
#include "asset_map.h"
#include "../util.h"
#include "../xml.h"

using std::string;
using std::list;
using boost::shared_ptr;
using namespace libdcp::parse;

AssetMap::AssetMap (string file)
{
	cxml::Document f ("AssetMap");
	f.read_file (file);

	id = f.string_child ("Id");
	creator = f.string_child ("Creator");
	volume_count = f.number_child<int64_t> ("VolumeCount");
	issue_date = f.string_child ("IssueDate");
	issuer = f.string_child ("Issuer");
	assets = type_grand_children<AssetMapAsset> (f, "AssetList", "Asset");
}

AssetMapAsset::AssetMapAsset (shared_ptr<const cxml::Node> node)
{
	id = node->string_child ("Id");
	packing_list = node->optional_string_child ("PackingList").get_value_or ("");
	chunks = type_grand_children<Chunk> (node, "ChunkList", "Chunk");
}

Chunk::Chunk (shared_ptr<const cxml::Node> node)
{
	path = node->string_child ("Path");

	string const prefix = "file://";

	if (boost::algorithm::starts_with (path, prefix)) {
		path = path.substr (prefix.length());
	}
	
	volume_index = node->optional_number_child<int64_t> ("VolumeIndex").get_value_or (0);
	offset = node->optional_number_child<int64_t> ("Offset").get_value_or (0);
	length = node->optional_number_child<int64_t> ("Length").get_value_or (0);
}

shared_ptr<AssetMapAsset>
AssetMap::asset_from_id (string id) const
{
	for (list<shared_ptr<AssetMapAsset> >::const_iterator i = assets.begin (); i != assets.end(); ++i) {
		if ((*i)->id == id) {
			return *i;
		}
	}

	return shared_ptr<AssetMapAsset> ();
}
