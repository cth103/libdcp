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

#include "types.h"
#include "raw_convert.h"
#include "font_node.h"
#include "xml.h"
#include "text_node.h"
#include <libcxml/cxml.h>
#include <boost/foreach.hpp>

using std::string;
using std::list;
using boost::shared_ptr;
using boost::optional;
using namespace dcp;

FontNode::FontNode (cxml::ConstNodePtr node, int tcr, string font_id_attribute)
{
	text = node->content ();

	id = node->optional_string_attribute (font_id_attribute);
	size = node->optional_number_attribute<int64_t> ("Size").get_value_or (0);
	aspect_adjust = node->optional_number_attribute<float> ("AspectAdjust");
	italic = node->optional_bool_attribute ("Italic");
	optional<string> c = node->optional_string_attribute ("Color");
	if (c) {
		colour = Colour (c.get ());
	}
	optional<string> const e = node->optional_string_attribute ("Effect");
	if (e) {
		effect = string_to_effect (e.get ());
	}
	c = node->optional_string_attribute ("EffectColor");
	if (c) {
		effect_colour = Colour (c.get ());
	}

	list<cxml::NodePtr> s = node->node_children ("Subtitle");
	BOOST_FOREACH (cxml::NodePtr& i, s) {
		subtitle_nodes.push_back (shared_ptr<SubtitleNode> (new SubtitleNode (i, tcr, font_id_attribute)));
	}

	list<cxml::NodePtr> f = node->node_children ("Font");
	BOOST_FOREACH (cxml::NodePtr& i, f) {
		font_nodes.push_back (shared_ptr<FontNode> (new FontNode (i, tcr, font_id_attribute)));
	}

	list<cxml::NodePtr> t = node->node_children ("Text");
	BOOST_FOREACH (cxml::NodePtr& i, t) {
		text_nodes.push_back (shared_ptr<TextNode> (new TextNode (i, tcr, font_id_attribute)));
	}
}

FontNode::FontNode (std::list<boost::shared_ptr<FontNode> > const & font_nodes)
	: size (0)
	, italic (false)
	, colour ("FFFFFFFF")
	, effect_colour ("FFFFFFFF")
{
	for (list<shared_ptr<FontNode> >::const_iterator i = font_nodes.begin(); i != font_nodes.end(); ++i) {
		if ((*i)->id) {
			id = (*i)->id;
		}
		if ((*i)->size != 0) {
			size = (*i)->size;
		}
		if ((*i)->aspect_adjust) {
			aspect_adjust = (*i)->aspect_adjust.get ();
		}
		if ((*i)->italic) {
			italic = (*i)->italic.get ();
		}
		if ((*i)->colour) {
			colour = (*i)->colour.get ();
		}
		if ((*i)->effect) {
			effect = (*i)->effect.get ();
		}
		if ((*i)->effect_colour) {
			effect_colour = (*i)->effect_colour.get ();
		}
	}
}
