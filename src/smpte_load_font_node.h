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

/** @file  src/smpte_load_font_node.h
 *  @brief SMPTELoadFontNode class.
 */

#include "load_font_node.h"
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

namespace cxml {
	class Node;
}

namespace dcp {

/** @class SMPTELoadFontNode
 *  @brief Parser for LoadFont nodes from SMPTE subtitle XML.
 */
class SMPTELoadFontNode : public LoadFontNode
{
public:
	SMPTELoadFontNode () {}
	SMPTELoadFontNode (std::string id, std::string urn);
	explicit SMPTELoadFontNode (boost::shared_ptr<const cxml::Node> node);

	std::string urn;
};

bool operator== (SMPTELoadFontNode const & a, SMPTELoadFontNode const & b);
bool operator!= (SMPTELoadFontNode const & a, SMPTELoadFontNode const & b);

}
