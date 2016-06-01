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

/** @file  src/load_font_node.h
 *  @brief LoadFontNode class.
 */

#include <string>

namespace dcp {

/** @class LoadFontNode
 *  @brief Parser for LoadFont nodes from subtitle XML.
 */
class LoadFontNode
{
public:
	LoadFontNode () {}
	LoadFontNode (std::string id_)
		: id (id_)
	{}

	virtual ~LoadFontNode () {}

	std::string id;
};

}
