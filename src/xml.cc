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
XMLNode::node_child (string name)
{
	list<xmlpp::Node*> n = node_children (name);
	if (n.size() > 1) {
		throw XMLError ("duplicate XML tag " + name);
	} else if (n.empty ()) {
		throw XMLError ("missing XML tag " + name + " in " + _node->get_name());
	}
	
	return n.front ();
}

list<xmlpp::Node*>
XMLNode::node_children (string name)
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
XMLNode::string_child (string name)
{
	return XMLNode (node_child (name)).content ();
}

string
XMLNode::optional_string_child (string name)
{
	list<xmlpp::Node*> nodes = node_children (name);
	if (nodes.size() > 2) {
		throw XMLError ("duplicate XML tag " + name);
	}

	if (nodes.empty ()) {
		return "";
	}

	return string_child (name);
}

ContentKind
XMLNode::kind_child (string name)
{
	return content_kind_from_string (string_child (name));
}

Fraction
XMLNode::fraction_child (string name)
{
	return Fraction (string_child (name));
}

int64_t
XMLNode::int64_child (string name)
{
	string s = string_child (name);
	erase_all (s, " ");
	return lexical_cast<int64_t> (s);
}

int64_t
XMLNode::optional_int64_child (string name)
{
	list<xmlpp::Node*> nodes = node_children (name);
	if (nodes.size() > 2) {
		throw XMLError ("duplicate XML tag " + name);
	}

	if (nodes.empty ()) {
		return 0;
	}

	return int64_child (name);
}

float
XMLNode::float_child (string name)
{
	return lexical_cast<float> (string_child (name));
}

void
XMLNode::ignore_child (string name)
{
	_taken.push_back (name);
}

Time
XMLNode::time_attribute (string name)
{
	return Time (string_attribute (name));
}

string
XMLNode::string_attribute (string name)
{
	xmlpp::Element const * e = dynamic_cast<const xmlpp::Element *> (_node);
	if (!e) {
		throw XMLError ("missing attribute");
	}
	
	xmlpp::Attribute* a = e->get_attribute (name);
	if (!a) {
		throw XMLError ("missing attribute");
	}

	return a->get_value ();
}

string
XMLNode::optional_string_attribute (string name)
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
	string const s = optional_string_attribute (name);
	if (s.empty ()) {
		return 0;
	}
	
	return lexical_cast<int64_t> (s);
}

optional<bool>
XMLNode::optional_bool_attribute (string name)
{
	string const s = optional_string_attribute (name);
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
	string const s = optional_string_attribute (name);
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
	string content;
	
        xmlpp::Node::NodeList c = _node->get_children ();
	for (xmlpp::Node::NodeList::const_iterator i = c.begin(); i != c.end(); ++i) {
		xmlpp::ContentNode const * v = dynamic_cast<xmlpp::ContentNode const *> (*i);
		if (v) {
			content += v->get_content ();
		}
	}

	return content;
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
