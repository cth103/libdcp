/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/smpte_load_font_node.cc
 *  @brief SMPTELoadFontNode class
 */


#include "smpte_load_font_node.h"
#include "util.h"
#include <libcxml/cxml.h>


using std::string;
using std::shared_ptr;
using namespace dcp;


SMPTELoadFontNode::SMPTELoadFontNode (string id, string urn_)
	: LoadFontNode (id)
	, urn (urn_)
{

}


SMPTELoadFontNode::SMPTELoadFontNode (shared_ptr<const cxml::Node> node)
	: LoadFontNode (node->string_attribute ("ID"))
	, urn (remove_urn_uuid (node->content()))
{

}


bool
dcp::operator== (SMPTELoadFontNode const & a, SMPTELoadFontNode const & b)
{
	return a.id == b.id && a.urn == b.urn;
}


bool
dcp::operator!= (SMPTELoadFontNode const & a, SMPTELoadFontNode const & b)
{
	return !(a == b);
}
