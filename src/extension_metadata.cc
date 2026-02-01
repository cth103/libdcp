/*
    Copyright (C) 2026 Carl Hetherington <cth@carlh.net>

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


#include "extension_metadata.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS


using std::string;
using std::vector;
using namespace dcp;


ExtensionMetadata::ExtensionMetadata(string scope_, string name_, vector<ExtensionMetadata::Property> properties_)
	: scope(std::move(scope_))
	, name(std::move(name_))
	, properties(std::move(properties_))
{

}


ExtensionMetadata::ExtensionMetadata(cxml::ConstNodePtr node)
	: scope(node->string_attribute("scope"))
	, name(node->optional_string_child("Name").get_value_or(""))
{
	if (auto list = node->optional_node_child("PropertyList")) {
		for (auto property: list->node_children("Property")) {
			properties.push_back(Property(property));
		}
	}
}


void
ExtensionMetadata::as_xml(xmlpp::Element* parent) const
{
	auto extension = cxml::add_child(parent, "ExtensionMetadata", string("meta"));
	extension->set_attribute("scope", scope);
	cxml::add_child(extension, "Name", string("meta"))->add_child_text(name);
	auto property_list = cxml::add_child(extension, "PropertyList", string("meta"));
	for (auto const& property: properties) {
		property.as_xml(cxml::add_child(property_list, "Property", string("meta")));
	}
}


ExtensionMetadata::Property::Property(cxml::ConstNodePtr node)
	: name(node->optional_string_child("Name").get_value_or(""))
	, value(node->optional_string_child("Value").get_value_or(""))
{

}


void
ExtensionMetadata::Property::as_xml(xmlpp::Element* parent) const
{
	cxml::add_child(parent, "Name", string("meta"))->add_child_text(name);
	cxml::add_child(parent, "Value", string("meta"))->add_child_text(value);
}


