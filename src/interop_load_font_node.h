/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#include "load_font_node.h"
#include <libcxml/cxml.h>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

namespace dcp {

class InteropLoadFontNode : public LoadFontNode
{
public:
	InteropLoadFontNode () {}
	InteropLoadFontNode (std::string id, std::string uri);
	explicit InteropLoadFontNode (cxml::ConstNodePtr node);

	std::string uri;
};

bool operator== (InteropLoadFontNode const & a, InteropLoadFontNode const & b);
bool operator!= (InteropLoadFontNode const & a, InteropLoadFontNode const & b);

}
