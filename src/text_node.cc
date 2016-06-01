/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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
*/

/** @file  src/text.cc
 *  @brief TextNode class for parsing subtitle XML.
 */

#include "text_node.h"
#include "xml.h"
#include "font_node.h"
#include <libcxml/cxml.h>
#include <boost/foreach.hpp>

using std::string;
using std::list;
using boost::shared_ptr;
using boost::optional;
using namespace dcp;

/** Read a &lt;Text&gt; node from a subtitle XML file, noting its contents
 *  in this object's member variables.
 *  @param node Node to read.
 */
TextNode::TextNode (boost::shared_ptr<const cxml::Node> node, int tcr, string font_id_attribute)
	: h_position (0)
	, h_align (HALIGN_CENTER)
	, v_position (0)
	, v_align (VALIGN_CENTER)
	, direction (DIRECTION_LTR)
{
	text = node->content ();

	optional<float> hp = node->optional_number_attribute<float> ("HPosition");
	if (!hp) {
		hp = node->optional_number_attribute<float> ("Hposition");
	}
	if (hp) {
		h_position = hp.get () / 100;
	}

	optional<string> ha = node->optional_string_attribute ("HAlign");
	if (!ha) {
		ha = node->optional_string_attribute ("Halign");
	}
	if (ha) {
		h_align = string_to_halign (ha.get ());
	}

	optional<float> vp = node->optional_number_attribute<float> ("VPosition");
	if (!vp) {
		vp = node->optional_number_attribute<float> ("Vposition");
	}
	if (vp) {
		v_position = vp.get () / 100;
	}

	optional<string> va = node->optional_string_attribute ("VAlign");
	if (!va) {
		va = node->optional_string_attribute ("Valign");
	}
	if (va) {
		v_align = string_to_valign (va.get ());
	}

	optional<string> d = node->optional_string_attribute ("Direction");
	if (d) {
		direction = string_to_direction (d.get ());
	}

	list<cxml::NodePtr> f = node->node_children ("Font");
	BOOST_FOREACH (cxml::NodePtr& i, f) {
		font_nodes.push_back (shared_ptr<FontNode> (new FontNode (i, tcr, font_id_attribute)));
	}
}
