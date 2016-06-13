/*
    Copyright (C) 2012-2016 Carl Hetherington <cth@carlh.net>

    This file is part of libdcp.

    libdcp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libdcp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdcp.  If not, see <http://www.gnu.org/licenses/>.

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
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

/** @param tcr Timecode rate for SMPTE, or empty for Interop */
SubtitleNode::SubtitleNode (boost::shared_ptr<const cxml::Node> node, optional<int> tcr, string font_id_attribute)
{
	in = Time (node->string_attribute ("TimeIn"), tcr);
	out = Time (node->string_attribute ("TimeOut"), tcr);

	list<cxml::NodePtr> f = node->node_children ("Font");
	for (list<cxml::NodePtr>::iterator i = f.begin(); i != f.end(); ++i) {
		font_nodes.push_back (shared_ptr<FontNode> (new FontNode (*i, tcr, font_id_attribute)));
	}

	list<cxml::NodePtr> t = node->node_children ("Text");
	for (list<cxml::NodePtr>::iterator i = t.begin(); i != t.end(); ++i) {
		text_nodes.push_back (shared_ptr<TextNode> (new TextNode (*i, tcr, font_id_attribute)));
	}

	fade_up_time = fade_time (node, "FadeUpTime", tcr);
	fade_down_time = fade_time (node, "FadeDownTime", tcr);
}

Time
SubtitleNode::fade_time (shared_ptr<const cxml::Node> node, string name, optional<int> tcr)
{
	string const u = node->optional_string_attribute (name).get_value_or ("");
	Time t;

	if (u.empty ()) {
		t = Time (0, 0, 0, 20, 250);
	} else if (u.find (":") != string::npos) {
		t = Time (u, tcr);
	} else {
		t = Time (0, 0, 0, lexical_cast<int> (u), tcr.get_value_or(250));
	}

	if (t > Time (0, 0, 8, 0, 250)) {
		t = Time (0, 0, 8, 0, 250);
	}

	return t;
}
