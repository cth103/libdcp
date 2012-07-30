#include "xml.h"

XMLFile::XMLFile (string file, string root_id)
{
	xmlpp::DomParser parser;
	parser.parse_file (file);
	if (!parser) {
		throw XMLError ("could not parse XML");
	}

	_root = parser.get_document()->get_root_node ();
	if (_root->get_name() != root_id) {
		throw XMLError ("unrecognised root node");
	}
}

string
XMLFile::string_tag (string id)
{
	stringstream x;
	x << _root->get_name() << "/" << id;
	xmlpp::NodeSet n = _root->find (x.str ());
	if (n.empty ()) {
		throw XMLError ("missing XML tag");
	} else if (n.size() > 1) {
		throw XMLError ("duplicate XML tag");
	}

	xml::Node::NodeList c = n.front()->get_children ();
	if (c.empty() 
	    

	return n.front()->get_name ();
}
