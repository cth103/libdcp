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

/** @file  src/text.h
 *  @brief TextNode class for parsing subtitle XML.
 */

#include "types.h"
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <list>

namespace cxml {
	class Node;
}

namespace dcp {

class FontNode;

/** @class TextNode
 *  @brief Parser for Text nodes from subtitle XML.
 */
class TextNode
{
public:
	/** Construct a default text node */
	TextNode ()
		: v_position (0)
		, v_align (TOP)
	{}
	
	TextNode (boost::shared_ptr<const cxml::Node> node, int tcr);

	float v_position;
	VAlign v_align;
	std::string text;
	std::list<boost::shared_ptr<FontNode> > font_nodes;
};

}
