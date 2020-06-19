/*
    Copyright (C) 2016-2017 Carl Hetherington <cth@carlh.net>

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

/** @file  src/reel_atmos_asset.cc
 *  @brief ReelAtmosAsset class.
 */

#include "atmos_asset.h"
#include "reel_atmos_asset.h"
#include <libcxml/cxml.h>
#include <libxml++/libxml++.h>

using std::string;
using std::pair;
using std::make_pair;
using boost::shared_ptr;
using namespace dcp;

ReelAtmosAsset::ReelAtmosAsset (boost::shared_ptr<AtmosAsset> asset, int64_t entry_point)
	: ReelAsset (asset->id(), asset->edit_rate(), asset->intrinsic_duration(), entry_point)
	, ReelMXF (asset, asset->key_id())
{

}

ReelAtmosAsset::ReelAtmosAsset (boost::shared_ptr<const cxml::Node> node)
	: ReelAsset (node)
	, ReelMXF (node)
{
	node->ignore_child ("DataType");
	node->done ();
}

string
ReelAtmosAsset::cpl_node_name (Standard) const
{
	return "axd:AuxData";
}

pair<string, string>
ReelAtmosAsset::cpl_node_namespace (Standard) const
{
	return make_pair ("http://www.dolby.com/schemas/2012/AD", "axd");
}

string
ReelAtmosAsset::key_type () const
{
	return "MDEK";
}

xmlpp::Node *
ReelAtmosAsset::write_to_cpl (xmlpp::Node* node, Standard standard) const
{
	xmlpp::Node* asset = write_to_cpl_asset (node, standard, hash());
	write_to_cpl_mxf (asset);
	asset->add_child("axd:DataType")->add_child_text("urn:smpte:ul:060e2b34.04010105.0e090604.00000000");
	return asset;
}

bool
ReelAtmosAsset::equals (shared_ptr<const ReelAtmosAsset> other, EqualityOptions opt, NoteHandler note) const
{
	if (!asset_equals (other, opt, note)) {
		return false;
	}
	if (!mxf_equals (other, opt, note)) {
		return false;
	}

	return true;
}
