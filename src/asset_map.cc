#include "asset_map.h"

using namespace std;
using namespace libdcp;

AssetMap::AssetMap (string file)
	: XMLFile (file)
{
	id = string_node ("Id");
	creator = string_node ("Creator");
	volume_count = int_node ("VolumeCount");
	issue_date = string_node ("IssueDate");
	issuer = string_node ("Issuer");
	asset_list = sub_node<AssetList> ("AssetMapAssetList");
}

AssetMapAssetList::AssetMapAssetList (xmlpp::Node const * node)
	: XMLNode (node)
{
	assets = sub_nodes<AssetMapAsset> ("Asset");
}

AssetMapAsset::AssetMapAsset (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_node ("Id");
	packing_list = optional_string_node ("PackingList");
	chunk_list = sub_node<ChunkList> ("ChunkList");
}

ChunkList::ChunkList (xmlpp::Node const * node)
	: XMLNode (node)
{
	chunks = sub_nodes<Chunk> ("Chunk");
}

Chunk::Chunk (xmlpp::Node const * node)
	: XMLNode (node)
{
	path = string_node ("Path");
	volume_index = int_node ("VolumeIndex");
	offset = int_node ("Offset");
	length = int_node ("Length");
}

