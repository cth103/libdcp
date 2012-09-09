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
#include "pkl_file.h"

using namespace std;
using namespace boost;
using namespace libdcp;

PKLFile::PKLFile (string file)
	: XMLFile (file, "PackingList")
{
	id = string_node ("Id");
	annotation_text = string_node ("AnnotationText");
	issue_date = string_node ("IssueDate");
	issuer = string_node ("Issuer");
	creator = string_node ("Creator");
	assets = sub_nodes<PKLAsset> ("AssetList", "Asset");
}

PKLAsset::PKLAsset (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_node ("Id");
	annotation_text = optional_string_node ("AnnotationText");
	hash = string_node ("Hash");
	size = int64_node ("Size");
	type = string_node ("Type");
	original_file_name = optional_string_node ("OriginalFileName");
}
