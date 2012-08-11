#include <iostream>
#include "pkl.h"

using namespace std;
using namespace boost;
using namespace libdcp;

PKL::PKL (string file)
	: XMLFile (file, "PackingList")
{
	id = string_node ("Id");
	annotation_text = string_node ("AnnotationText");
	issue_date = string_node ("IssueDate");
	issuer = string_node ("Issuer");
	creator = string_node ("Creator");
	assets = sub_nodes<PKLAsset> ("AssetList", "Asset");
}

PKLAsset::PKLAsset (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_node ("Id");
	annotation_text = optional_string_node ("AnnotationText");
	hash = string_node ("Hash");
	size = int64_node ("Size");
	type = string_node ("Type");
	original_file_name = optional_string_node ("OriginalFileName");
}
	
shared_ptr<PKLAsset>
PKL::asset_from_id (string id) const
{
	for (list<shared_ptr<PKLAsset> >::const_iterator i = assets.begin (); i != assets.end(); ++i) {
		if ((*i)->id == id) {
			return *i;
		}
	}

	return shared_ptr<PKLAsset> ();
}

