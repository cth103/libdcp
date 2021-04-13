/*
    Copyright (C) 2019-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/reel_markers_asset.cc
 *  @brief ReelMarkersAsset class
 */


#include "dcp_assert.h"
#include "raw_convert.h"
#include "reel_markers_asset.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS


using std::string;
using std::map;
using std::max;
using boost::optional;
using std::shared_ptr;
using namespace dcp;


ReelMarkersAsset::ReelMarkersAsset (Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point)
	: ReelAsset (make_uuid(), edit_rate, intrinsic_duration, entry_point)
{

}


ReelMarkersAsset::ReelMarkersAsset (cxml::ConstNodePtr node)
	: ReelAsset (node)
{
	auto list = node->node_child ("MarkerList");
	DCP_ASSERT (list);
	for (auto i: list->node_children("Marker")) {
		set (marker_from_string(i->string_child("Label")), dcp::Time(i->number_child<int64_t>("Offset"), edit_rate().as_float(), edit_rate().numerator));
	}
}


string
ReelMarkersAsset::cpl_node_name (Standard) const
{
	return "MainMarkers";
}


void
ReelMarkersAsset::set (Marker m, Time t)
{
	_markers[m] = t;
}


void
ReelMarkersAsset::unset (Marker m)
{
	_markers.erase (m);
}


optional<Time>
ReelMarkersAsset::get (Marker m) const
{
	auto i = _markers.find (m);
	if (i == _markers.end ()) {
		return {};
	}
	return i->second;
}


xmlpp::Node*
ReelMarkersAsset::write_to_cpl (xmlpp::Node* node, Standard standard) const
{
	int const tcr = edit_rate().numerator / edit_rate().denominator;
	auto asset = ReelAsset::write_to_cpl (node, standard);
	auto ml = asset->add_child("MarkerList");
	for (auto const& i: _markers) {
		auto m = ml->add_child("Marker");
		m->add_child("Label")->add_child_text(marker_to_string(i.first));
		m->add_child("Offset")->add_child_text(raw_convert<string>(i.second.as_editable_units_ceil(tcr)));
	}

	return asset;
}

bool
ReelMarkersAsset::equals (shared_ptr<const ReelMarkersAsset> other, EqualityOptions opt, NoteHandler note) const
{
	if (!asset_equals(other, opt, note)) {
		return false;
	}

	if (get(Marker::FFOC) != other->get(Marker::FFOC) ||
	    get(Marker::LFOC) != other->get(Marker::LFOC) ||
	    get(Marker::FFTC) != other->get(Marker::FFTC) ||
	    get(Marker::LFTC) != other->get(Marker::LFTC) ||
	    get(Marker::FFOI) != other->get(Marker::FFOI) ||
	    get(Marker::LFOI) != other->get(Marker::LFOI) ||
	    get(Marker::FFEC) != other->get(Marker::FFEC) ||
	    get(Marker::LFEC) != other->get(Marker::LFEC) ||
	    get(Marker::FFMC) != other->get(Marker::FFMC) ||
	    get(Marker::LFMC) != other->get(Marker::LFMC)) {
		return false;
	}

	return true;
}
