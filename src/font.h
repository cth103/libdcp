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

/** @file  src/font.h
 *  @brief Font class
 */

#include "types.h"
#include "subtitle.h"
#include <libcxml/cxml.h>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <list>

namespace dcp {

/** @class Font
 *  @brief Helper class for parsing subtitle XML.
 */
class Font 
{
public:
	Font ()
		: size (0)
	{}
	
	Font (cxml::ConstNodePtr node, int tcr);
	Font (std::list<boost::shared_ptr<Font> > const & font_nodes);

	std::string text;
	boost::optional<std::string> id;
	int size;
	boost::optional<bool> italic;
	boost::optional<Colour> colour;
	boost::optional<Effect> effect;
	boost::optional<Colour> effect_colour;
	
	std::list<boost::shared_ptr<Subtitle> > subtitle_nodes;
	std::list<boost::shared_ptr<Font> > font_nodes;
	std::list<boost::shared_ptr<Text> > text_nodes;
};

}
