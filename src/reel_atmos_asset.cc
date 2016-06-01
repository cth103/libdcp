/*
    Copyright (C) 2016 Carl Hetherington <cth@carlh.net>

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

/** @file  src/reel_atmos_asset.cc
 *  @brief ReelAtmosAsset class.
 */

#include "atmos_asset.h"
#include "reel_atmos_asset.h"
#include <libcxml/cxml.h>
#include <libxml++/libxml++.h>

using std::string;
using boost::shared_ptr;
using namespace dcp;

ReelAtmosAsset::ReelAtmosAsset (boost::shared_ptr<AtmosAsset> asset, int64_t entry_point)
	: ReelAsset (asset, asset->edit_rate(), asset->intrinsic_duration(), entry_point)
{

}

ReelAtmosAsset::ReelAtmosAsset (boost::shared_ptr<const cxml::Node> node)
	: ReelAsset (node)
{
	node->done ();
}

string
ReelAtmosAsset::cpl_node_name () const
{
	return "axd:AuxData";
}

string
ReelAtmosAsset::key_type () const
{
	return "MDEK";
}

void
ReelAtmosAsset::write_to_cpl (xmlpp::Node* node, Standard standard) const
{
	ReelAsset::write_to_cpl (node, standard);

	/* Find <axd:AuxData> */
	xmlpp::Node* mp = find_child (node, cpl_node_name ());
	mp->add_child("axd:DataType")->add_child_text ("urn:smpte:ul:060e2b34.04010105.0e090604.00000000");
}
