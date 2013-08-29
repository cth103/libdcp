/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

/** @file  src/pkl_file.cc
 *  @brief Classes used to parse a PKL.
 */

#include <iostream>
#include "pkl.h"

using namespace std;
using namespace boost;
using namespace libdcp::parse;

PKL::PKL (string file)
{
	cxml::Document f ("PackingList");
	f.read_file (file);
	
	id = f.string_child ("Id");
	annotation_text = f.optional_string_child ("AnnotationText").get_value_or ("");
	issue_date = f.string_child ("IssueDate");
	issuer = f.string_child ("Issuer");
	creator = f.string_child ("Creator");
	assets = type_grand_children<PKLAsset> (f, "AssetList", "Asset");
}

PKLAsset::PKLAsset (boost::shared_ptr<const cxml::Node> node)
{
	id = node->string_child ("Id");
	annotation_text = node->optional_string_child ("AnnotationText").get_value_or ("");
	hash = node->string_child ("Hash");
	size = node->number_child<int64_t> ("Size");
	type = node->string_child ("Type");
	original_file_name = node->optional_string_child ("OriginalFileName").get_value_or ("");
}
