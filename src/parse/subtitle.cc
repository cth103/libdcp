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

#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include "subtitle.h"
#include "../raw_convert.h"
#include "../types.h"

using std::string;
using std::list;
using boost::shared_ptr;
using boost::optional;
using namespace libdcp;
using namespace libdcp::parse;

Font::Font (shared_ptr<const cxml::Node> node, optional<int> tcr)
{
	text = node->content ();
	
	id = node->optional_string_attribute ("Id").get_value_or ("");
	size = node->optional_number_attribute<int64_t> ("Size").get_value_or (0);
	italic = node->optional_bool_attribute ("Italic");
	optional<string> c = node->optional_string_attribute ("Color");
	if (c) {
		color = Color (c.get ());
	}
	optional<string> const e = node->optional_string_attribute ("Effect");
	if (e) {
		effect = string_to_effect (e.get ());
	}
	c = node->optional_string_attribute ( "EffectColor");
	if (c) {
		effect_color = Color (c.get ());
	}

	list<cxml::NodePtr> s = node->node_children ("Subtitle");
	for (list<cxml::NodePtr>::iterator i = s.begin(); i != s.end(); ++i) {
		subtitle_nodes.push_back (shared_ptr<Subtitle> (new Subtitle (*i, tcr)));
	}

	list<cxml::NodePtr> f = node->node_children ("Font");
	for (list<cxml::NodePtr>::iterator i = f.begin(); i != f.end(); ++i) {
		font_nodes.push_back (shared_ptr<Font> (new Font (*i, tcr)));
	}

	list<cxml::NodePtr> t = node->node_children ("Text");
	for (list<cxml::NodePtr>::iterator i = t.begin(); i != t.end(); ++i) {
		text_nodes.push_back (shared_ptr<Text> (new Text (*i, tcr)));
	}
}

Font::Font (list<shared_ptr<Font> > const & font_nodes)
	: size (0)
	, italic (false)
	, color ("FFFFFFFF")
	, effect_color ("FFFFFFFF")
{
	for (list<shared_ptr<Font> >::const_iterator i = font_nodes.begin(); i != font_nodes.end(); ++i) {
		if (!(*i)->id.empty ()) {
			id = (*i)->id;
		}
		if ((*i)->size != 0) {
			size = (*i)->size;
		}
		if ((*i)->italic) {
			italic = (*i)->italic.get ();
		}
		if ((*i)->color) {
			color = (*i)->color.get ();
		}
		if ((*i)->effect) {
			effect = (*i)->effect.get ();
		}
		if ((*i)->effect_color) {
			effect_color = (*i)->effect_color.get ();
		}
	}
}

LoadFont::LoadFont (shared_ptr<const cxml::Node> node)
{
	optional<string> x = node->optional_string_attribute ("Id");
	if (!x) {
		x = node->optional_string_attribute ("ID");
	}
	id = x.get_value_or ("");
		
	uri = node->optional_string_attribute ("URI");
}

/** @param tcr A timecode rate, if this subtitle is from a SMPTE file, or empty if it is Interop */
Subtitle::Subtitle (shared_ptr<const cxml::Node> node, optional<int> tcr)
{
	in = Time (node->string_attribute ("TimeIn"), tcr.get_value_or (250));
	out = Time (node->string_attribute ("TimeOut"), tcr.get_value_or (250));

	list<cxml::NodePtr> f = node->node_children ("Font");
	for (list<cxml::NodePtr>::iterator i = f.begin(); i != f.end(); ++i) {
		font_nodes.push_back (shared_ptr<Font> (new Font (*i, tcr)));
	}

	list<cxml::NodePtr> t = node->node_children ("Text");
	for (list<cxml::NodePtr>::iterator i = t.begin(); i != t.end(); ++i) {
		text_nodes.push_back (shared_ptr<Text> (new Text (*i, tcr)));
	}
	
	fade_up_time = fade_time (node, "FadeUpTime", tcr);
	fade_down_time = fade_time (node, "FadeDownTime", tcr);
}

Time
Subtitle::fade_time (shared_ptr<const cxml::Node> node, string name, optional<int> tcr)
{
	string const u = node->optional_string_attribute (name).get_value_or ("");
	Time t;
	
	if (u.empty ()) {
		t = Time (0, 0, 0, 20, 250);
	} else if (u.find (":") != string::npos) {
		t = Time (u, tcr.get_value_or(250));
	} else {
		t = Time (0, 0, 0, raw_convert<int> (u), tcr.get_value_or(250));
	}

	if (t > Time (0, 0, 8, 0, 250)) {
		t = Time (0, 0, 8, 0, 250);
	}

	return t;
}

Text::Text (shared_ptr<const cxml::Node> node, optional<int> tcr)
	: v_align (VERTICAL_CENTER)
	, h_align (HORIZONTAL_CENTER)
{
	/* Vertical position */
	text = node->content ();
	optional<float> x = node->optional_number_attribute<float> ("VPosition");
	if (!x) {
		x = node->number_attribute<float> ("Vposition");
	}
	v_position = x.get ();

	/* Vertical alignment */
	optional<string> v = node->optional_string_attribute ("VAlign");
	if (!v) {
		v = node->optional_string_attribute ("Valign");
	}
	if (v) {
		v_align = string_to_valign (v.get ());
	}

	/* Horizontal alignment */
	optional<string> h = node->optional_string_attribute ("HAlign");
	if (!h) {
		h = node->optional_string_attribute ("Halign");
	}
	if (h) {
		h_align = string_to_halign (h.get ());
	}

	list<cxml::NodePtr> f = node->node_children ("Font");
	for (list<cxml::NodePtr>::iterator i = f.begin(); i != f.end(); ++i) {
		font_nodes.push_back (shared_ptr<Font> (new Font (*i, tcr)));
	}
}

