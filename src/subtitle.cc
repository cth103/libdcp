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

#include "subtitle.h"
#include "xml.h"
#include "font.h"
#include "text.h"
#include <libcxml/cxml.h>
#include <boost/lexical_cast.hpp>

using std::string;
using boost::shared_ptr;
using boost::lexical_cast;
using namespace dcp;

Subtitle::Subtitle (boost::shared_ptr<const cxml::Node> node)
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
		t = Time (0, 0, 0, lexical_cast<int> (u));
	}

	if (t > Time (0, 0, 8, 0)) {
		t = Time (0, 0, 8, 0);
	}

	return t;
}
