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

#include "text.h"
#include "xml.h"
#include "font.h"
#include <libcxml/cxml.h>

using std::string;
using boost::shared_ptr;
using boost::optional;
using namespace dcp;

Text::Text (shared_ptr<const cxml::Node> node)
	: v_align (CENTER)
{
	text = node->content ();
	v_position = node->number_attribute<float> ("VPosition");
	optional<string> v = node->optional_string_attribute ("VAlign");
	if (v) {
		v_align = string_to_valign (v.get ());
	}

	font_nodes = type_children<Font> (node, "Font");
}
