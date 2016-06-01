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

#include "interop_load_font_node.h"
#include <libcxml/cxml.h>

using std::string;
using boost::shared_ptr;
using boost::optional;
using namespace dcp;

InteropLoadFontNode::InteropLoadFontNode (string id_, string uri_)
	: LoadFontNode (id_)
	, uri (uri_)
{

}

InteropLoadFontNode::InteropLoadFontNode (cxml::ConstNodePtr node)
{
	optional<string> x = node->optional_string_attribute ("Id");
	if (!x) {
		x = node->optional_string_attribute ("ID");
	}
	id = x.get_value_or ("");

	uri = node->string_attribute ("URI");
}

bool
dcp::operator== (InteropLoadFontNode const & a, InteropLoadFontNode const & b)
{
	return a.id == b.id && a.uri == b.uri;
}

bool
dcp::operator!= (InteropLoadFontNode const & a, InteropLoadFontNode const & b)
{
	return !(a == b);
}
