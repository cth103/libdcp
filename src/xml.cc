#include <sstream>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <libxml++/libxml++.h>
#include "xml.h"
#include "exceptions.h"
#include "util.h"

using namespace std;
using namespace boost;
using namespace libdcp;

XMLNode::XMLNode ()
	: _node (0)
{

}

XMLNode::XMLNode (xmlpp::Node const * node)
	: _node (node)
{
	
}

xmlpp::Node *
XMLNode::xml_node (string name)
{
	list<xmlpp::Node*> n = xml_nodes (name);
	if (n.size() > 1) {
		throw XMLError ("duplicate XML tag " + name);
	} else if (n.empty ()) {
		throw XMLError ("missing XML tag " + name);
	}
	
	return n.front ();
}

list<xmlpp::Node*>
XMLNode::xml_nodes (string name)
{
	/* XXX: using find / get_path should work here, but I can't follow
	   how get_path works.
	*/

	xmlpp::Node::NodeList c = _node->get_children ();
	
	list<xmlpp::Node*> n;
	for (xmlpp::Node::NodeList::iterator i = c.begin (); i != c.end(); ++i) {
		if ((*i)->get_name() == name) {
			n.push_back (*i);
		}
	}
	
	_taken.push_back (name);
	return n;
}

string
XMLNode::string_node (string name)
{
	return XMLNode (xml_node (name)).content ();
}

string
XMLNode::optional_string_node (string name)
{
	list<xmlpp::Node*> nodes = xml_nodes (name);
	if (nodes.size() > 2) {
		throw XMLError ("duplicate XML tag " + name);
	}

	if (nodes.empty ()) {
		return "";
	}

	return string_node (name);
}

ContentKind
XMLNode::kind_node (string name)
{
	return content_kind_from_string (string_node (name));
}

Fraction
XMLNode::fraction_node (string name)
{
	return Fraction (string_node (name));
}

int64_t
XMLNode::int64_node (string name)
{
	return lexical_cast<int64_t> (string_node (name));
}

int64_t
XMLNode::optional_int64_node (string name)
{
	list<xmlpp::Node*> nodes = xml_nodes (name);
	if (nodes.size() > 2) {
		throw XMLError ("duplicate XML tag " + name);
	}

	if (nodes.empty ()) {
		return 0;
	}

	return int64_node (name);
}

float
XMLNode::float_node (string name)
{
	return lexical_cast<float> (string_node (name));
}

void
XMLNode::ignore_node (string name)
{
	_taken.push_back (name);
}

Time
XMLNode::time_attribute (string name)
{
	string const t = string_attribute (name);

	vector<string> b;
	boost::split (b, t, is_any_of (":"));
	assert (b.size() == 4);

	return Time (lexical_cast<int> (b[0]), lexical_cast<int> (b[1]), lexical_cast<int> (b[2]), lexical_cast<int> (b[3]));
}

string
XMLNode::string_attribute (string name)
{
	xmlpp::Element const * e = dynamic_cast<const xmlpp::Element *> (_node);
	if (!e) {
		return "";
	}
	
	xmlpp::Attribute* a = e->get_attribute (name);
	if (!a) {
		return "";
	}

	return a->get_value ();
}

float
XMLNode::float_attribute (string name)
{
	return lexical_cast<float> (string_attribute (name));
}

int64_t
XMLNode::int64_attribute (string name)
{
	return lexical_cast<int64_t> (string_attribute (name));
}

int64_t
XMLNode::optional_int64_attribute (string name)
{
	string const s = string_attribute (name);
	if (s.empty ()) {
		return 0;
	}
	
	return lexical_cast<int64_t> (s);
}

optional<bool>
XMLNode::optional_bool_attribute (string name)
{
	string const s = string_attribute (name);
	if (s.empty ()) {
		return optional<bool> ();
	}

	if (s == "1" || s == "yes") {
		return optional<bool> (true);
	}

	return optional<bool> (false);
}

optional<Color>
XMLNode::optional_color_attribute (string name)
{
	string const s = string_attribute (name);
	if (s.empty ()) {
		return optional<Color> ();
	}

	return optional<Color> (Color (s));
}

void
XMLNode::done ()
{
	xmlpp::Node::NodeList c = _node->get_children ();
	for (xmlpp::Node::NodeList::iterator i = c.begin(); i != c.end(); ++i) {
		if (dynamic_cast<xmlpp::Element *> (*i) && find (_taken.begin(), _taken.end(), (*i)->get_name()) == _taken.end ()) {
			throw XMLError ("unexpected XML node " + (*i)->get_name());
		}
	}
}

string
XMLNode::content ()
{
        xmlpp::Node::NodeList c = _node->get_children ();

	if (c.size() > 1) {
		throw XMLError ("unexpected content in XML node");
	}

	if (c.empty ()) {
		return "";
	}
	
        xmlpp::ContentNode const * v = dynamic_cast<xmlpp::ContentNode const *> (c.front());
	if (!v) {
		throw XMLError ("missing content in XML node");
	}
	
	return v->get_content ();
}

XMLFile::XMLFile (string file, string root_name)
{
	if (!filesystem::exists (file)) {
		throw FileError ("XML file does not exist", file);
	}
	
	_parser = new xmlpp::DomParser;
	_parser->parse_file (file);
	if (!_parser) {
		throw XMLError ("could not parse XML");
	}

	_node = _parser->get_document()->get_root_node ();
	if (_node->get_name() != root_name) {
		throw XMLError ("unrecognised root node");
	}
}

XMLFile::~XMLFile ()
{
	delete _parser;
}
