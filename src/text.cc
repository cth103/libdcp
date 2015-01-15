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

/** @file  src/text.cc
 *  @brief Text class for parsing subtitle XML.
 */

#include "text.h"
#include "xml.h"
#include "font.h"
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
Text::Text (boost::shared_ptr<const cxml::Node> node, int tcr)
	: v_align (CENTER)
{
	text = node->content ();
	optional<float> x = node->optional_number_attribute<float> ("VPosition");
	if (!x) {
		x = node->number_attribute<float> ("Vposition");
	}
	v_position = x.get () / 100;
	
	optional<string> v = node->optional_string_attribute ("VAlign");
	if (!v) {
		v = node->optional_string_attribute ("Valign");
	}
	
	if (v) {
		v_align = string_to_valign (v.get ());
	}

	list<cxml::NodePtr> f = node->node_children ("Font");
	BOOST_FOREACH (cxml::NodePtr& i, f) {
		font_nodes.push_back (shared_ptr<Font> (new Font (i, tcr)));
	}
}
