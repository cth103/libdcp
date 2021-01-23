/*
    Copyright (C) 2013-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/xml.h
 *  @brief Helpers for XML reading with libcxml
 */


#ifndef LIBDCP_XML_H
#define LIBDCP_XML_H


#include "exceptions.h"
#include <libcxml/cxml.h>


namespace dcp
{


template <class T>
std::shared_ptr<T>
optional_type_child (cxml::Node const & node, std::string name)
{
	auto n = node.node_children (name);
	if (n.size() > 1) {
		throw XMLError ("duplicate XML tag");
	} else if (n.empty ()) {
		return {};
	}

	return std::make_shared<T>(n.front());
}


template <class T>
std::shared_ptr<T> type_child (std::shared_ptr<const cxml::Node> node, std::string name) {
	return std::make_shared<T>(node->node_child(name));
}


template <class T>
std::shared_ptr<T>
optional_type_child (std::shared_ptr<const cxml::Node> node, std::string name)
{
	return optional_type_child<T> (*node.get(), name);
}


template <class T>
std::vector<std::shared_ptr<T>>
type_children (cxml::Node const & node, std::string name)
{
        std::vector<std::shared_ptr<T>> r;
	for (auto i: node.node_children(name)) {
		r.push_back (std::make_shared<T>(i));
	}
	return r;
}


template <class T>
std::vector<std::shared_ptr<T>>
type_children (std::shared_ptr<const cxml::Node> node, std::string name)
{
	return type_children<T> (*node.get(), name);
}


template <class T>
std::vector<std::shared_ptr<T>>
type_grand_children (cxml::Node const & node, std::string name, std::string sub)
{
	return type_children<T> (node.node_child(name), sub);
}


template <class T>
std::vector<std::shared_ptr<T>>
type_grand_children (std::shared_ptr<const cxml::Node> node, std::string name, std::string sub)
{
	return type_grand_children<T> (*node.get(), name, sub);
}

}


#endif
