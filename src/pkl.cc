#include "pkl.h"

using namespace std;
using namespace libdcp;

PKL::PKL (string file)
	: XMLFile (file, "PackingList")
{
	id = string_node ("Id");
	annotation_text = string_node ("AnnotationText");
	issue_date = string_node ("IssueDate");
	issuer = string_node ("Issuer");
	creator = string_node ("Creator");
	asset_list = sub_node<PKLAssetList> ("AssetList");
}

PKLAssetList::PKLAssetList (xmlpp::Node const * node)
	: XMLNode (node)
{
	assets = sub_nodes<PKLAsset> ("Asset");
}

PKLAsset::PKLAsset (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_node ("Id");
	annotation_text = optional_string_node ("AnnotationText");
	hash = string_node ("Hash");
	size = int_node ("Size");
	type = string_node ("Type");
}
	
