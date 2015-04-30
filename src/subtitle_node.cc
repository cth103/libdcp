/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

#include "subtitle_node.h"
#include "xml.h"
#include "font_node.h"
#include "text_node.h"
#include <libcxml/cxml.h>
#include <boost/lexical_cast.hpp>

using std::string;
using std::list;
using boost::optional;
using boost::shared_ptr;
using boost::lexical_cast;
using namespace dcp;

SubtitleNode::SubtitleNode (boost::shared_ptr<const cxml::Node> node, int tcr)
{
	in = Time (node->string_attribute ("TimeIn"), tcr);
	out = Time (node->string_attribute ("TimeOut"), tcr);

	list<cxml::NodePtr> f = node->node_children ("Font");
	for (list<cxml::NodePtr>::iterator i = f.begin(); i != f.end(); ++i) {
		font_nodes.push_back (shared_ptr<FontNode> (new FontNode (*i, tcr)));
	}

	list<cxml::NodePtr> t = node->node_children ("Text");
	for (list<cxml::NodePtr>::iterator i = t.begin(); i != t.end(); ++i) {
		text_nodes.push_back (shared_ptr<TextNode> (new TextNode (*i, tcr)));
	}
	
	fade_up_time = fade_time (node, "FadeUpTime", tcr);
	fade_down_time = fade_time (node, "FadeDownTime", tcr);
}

Time
SubtitleNode::fade_time (shared_ptr<const cxml::Node> node, string name, int tcr)
{
	string const u = node->optional_string_attribute (name).get_value_or ("");
	Time t;
	
	if (u.empty ()) {
		t = Time (0, 0, 0, 20, 250);
	} else if (u.find (":") != string::npos) {
		t = Time (u, tcr);
	} else {
		t = Time (0, 0, 0, lexical_cast<int> (u), tcr);
	}

	if (t > Time (0, 0, 8, 0, 250)) {
		t = Time (0, 0, 8, 0, 250);
	}

	return t;
}
