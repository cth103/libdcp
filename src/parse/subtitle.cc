/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

Font::Font (shared_ptr<const cxml::Node> node)
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
	subtitle_nodes = type_children<Subtitle> (node, "Subtitle");
	font_nodes = type_children<Font> (node, "Font");
	text_nodes = type_children<Text> (node, "Text");
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

Subtitle::Subtitle (shared_ptr<const cxml::Node> node)
{
	in = Time (node->string_attribute ("TimeIn"));
	out = Time (node->string_attribute ("TimeOut"));
	font_nodes = type_children<Font> (node, "Font");
	text_nodes = type_children<Text> (node, "Text");
	fade_up_time = fade_time (node, "FadeUpTime");
	fade_down_time = fade_time (node, "FadeDownTime");
}

Time
Subtitle::fade_time (shared_ptr<const cxml::Node> node, string name)
{
	string const u = node->optional_string_attribute (name).get_value_or ("");
	Time t;
	
	if (u.empty ()) {
		t = Time (0, 0, 0, 20);
	} else if (u.find (":") != string::npos) {
		t = Time (u);
	} else {
		t = Time (0, 0, 0, raw_convert<int> (u));
	}

	if (t > Time (0, 0, 8, 0)) {
		t = Time (0, 0, 8, 0);
	}

	return t;
}

Text::Text (shared_ptr<const cxml::Node> node)
	: v_align (CENTER)
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

	font_nodes = type_children<Font> (node, "Font");
}

