#include "asset_map.h"

using namespace std;
using namespace libdcp;

AssetMap::AssetMap (string file)
	: XMLFile (file, "AssetMap")
{
	id = string_node ("Id");
	creator = string_node ("Creator");
	volume_count = int_node ("VolumeCount");
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
	volume_index = int_node ("VolumeIndex");
	offset = int_node ("Offset");
	length = int_node ("Length");
}

