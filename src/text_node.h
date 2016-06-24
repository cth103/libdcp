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
		: h_position (0)
		, h_align (HALIGN_LEFT)
		, v_position (0)
		, v_align (VALIGN_TOP)
		, direction (DIRECTION_LTR)
	{}

	TextNode (boost::shared_ptr<const cxml::Node> node, boost::optional<int> tcr, Standard standard);

	float h_position;
	HAlign h_align;
	float v_position;
	VAlign v_align;
	Direction direction;
	std::string text;
	std::list<boost::shared_ptr<FontNode> > font_nodes;
};

}
