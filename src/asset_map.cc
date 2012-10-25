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

#include "asset_map.h"
#include "util.h"

using std::string;
using std::list;
using boost::shared_ptr;
using namespace libdcp;

AssetMap::AssetMap (string file)
	: XMLFile (file, "AssetMap")
{
	id = string_child ("Id");
	creator = string_child ("Creator");
	volume_count = int64_child ("VolumeCount");
	issue_date = string_child ("IssueDate");
	issuer = string_child ("Issuer");
	assets = type_grand_children<AssetMapAsset> ("AssetList", "Asset");
}

AssetMapAsset::AssetMapAsset (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_child ("Id");
	packing_list = optional_string_child ("PackingList");
	chunks = type_grand_children<Chunk> ("ChunkList", "Chunk");
}

Chunk::Chunk (xmlpp::Node const * node)
	: XMLNode (node)
{
	path = string_child ("Path");

	string const prefix = "file://";

	if (starts_with (path, prefix)) {
		path = path.substr (prefix.length());
	}
	
	volume_index = optional_int64_child ("VolumeIndex");
	offset = optional_int64_child ("Offset");
	length = optional_int64_child ("Length");
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
