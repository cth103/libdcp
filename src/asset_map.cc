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

using namespace std;
using namespace libdcp;

AssetMap::AssetMap (string file)
	: XMLFile (file, "AssetMap")
{
	id = string_node ("Id");
	creator = string_node ("Creator");
	volume_count = int64_node ("VolumeCount");
	issue_date = string_node ("IssueDate");
	issuer = string_node ("Issuer");
	assets = sub_nodes<AssetMapAsset> ("AssetList", "Asset");
}

AssetMapAsset::AssetMapAsset (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_node ("Id");
	packing_list = optional_string_node ("PackingList");
	chunks = sub_nodes<Chunk> ("ChunkList", "Chunk");
}

Chunk::Chunk (xmlpp::Node const * node)
	: XMLNode (node)
{
	path = string_node ("Path");
	volume_index = optional_int64_node ("VolumeIndex");
	offset = optional_int64_node ("Offset");
	length = optional_int64_node ("Length");
}

