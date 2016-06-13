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

/** @file  src/font_node.h
 *  @brief FontNode class
 */

#include "types.h"
#include "subtitle_node.h"
#include <libcxml/cxml.h>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <list>

namespace dcp {

/** @class FontNode
 *  @brief Helper class for parsing subtitle XML.
 */
class FontNode
{
public:
	FontNode ()
		: size (0)
	{}

	FontNode (cxml::ConstNodePtr node, int tcr, std::string font_id_attribute);
	explicit FontNode (std::list<boost::shared_ptr<FontNode> > const & font_nodes);

	std::string text;
	boost::optional<std::string> id;
	int size;
	boost::optional<float> aspect_adjust;
	boost::optional<bool> italic;
	boost::optional<bool> bold;
	boost::optional<Colour> colour;
	boost::optional<Effect> effect;
	boost::optional<Colour> effect_colour;

	std::list<boost::shared_ptr<SubtitleNode> > subtitle_nodes;
	std::list<boost::shared_ptr<FontNode> > font_nodes;
	std::list<boost::shared_ptr<TextNode> > text_nodes;
};

}
