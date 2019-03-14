/*
    Copyright (C) 2014-2015 Carl Hetherington <cth@carlh.net>

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

/** @file  src/reel_asset.cc
 *  @brief ReelAsset class.
 */

#include "raw_convert.h"
#include "reel_asset.h"
#include "asset.h"
#include "compose.hpp"
#include <libcxml/cxml.h>
#include <libxml++/libxml++.h>

using std::pair;
using std::string;
using std::make_pair;
using boost::shared_ptr;
using namespace dcp;

/** Construct a ReelAsset.
 *  @param asset Asset that this ReelAsset refers to.
 *  @param edit_rate Edit rate for the asset.
 *  @param intrinsic_duration Intrinsic duration of this asset.
 *  @param entry_point Entry point to use in that asset.
 */
ReelAsset::ReelAsset (shared_ptr<Asset> asset, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point)
	: Object (asset->id ())
	, _asset_ref (asset)
	, _edit_rate (edit_rate)
	, _intrinsic_duration (intrinsic_duration)
	, _entry_point (entry_point)
	, _duration (intrinsic_duration - entry_point)
	, _hash (asset->hash ())
{
	/* default _annotation_text to the leaf name of our file */
	if (asset->file ()) {
		_annotation_text = asset->file()->leaf().string ();
	}
}

ReelAsset::ReelAsset (shared_ptr<const cxml::Node> node)
	: Object (remove_urn_uuid (node->string_child ("Id")))
	, _asset_ref (_id)
	, _annotation_text (node->optional_string_child ("AnnotationText").get_value_or (""))
	, _edit_rate (Fraction (node->string_child ("EditRate")))
	, _intrinsic_duration (node->number_child<int64_t> ("IntrinsicDuration"))
	, _entry_point (node->number_child<int64_t> ("EntryPoint"))
	, _duration (node->number_child<int64_t> ("Duration"))
	, _hash (node->optional_string_child ("Hash"))
{

}

xmlpp::Node*
ReelAsset::write_to_cpl (xmlpp::Node* node, Standard standard) const
{
        xmlpp::Element* a = node->add_child (cpl_node_name (standard));
        pair<string, string> const attr = cpl_node_attribute (standard);
        if (!attr.first.empty ()) {
                a->set_attribute (attr.first, attr.second);
        }
        pair<string, string> const ns = cpl_node_namespace (standard);
        if (!ns.first.empty ()) {
                a->set_namespace_declaration (ns.first, ns.second);
        }
        a->add_child("Id")->add_child_text ("urn:uuid:" + _id);
        a->add_child("AnnotationText")->add_child_text (_annotation_text);
        a->add_child("EditRate")->add_child_text (String::compose ("%1 %2", _edit_rate.numerator, _edit_rate.denominator));
        a->add_child("IntrinsicDuration")->add_child_text (raw_convert<string> (_intrinsic_duration));
        a->add_child("EntryPoint")->add_child_text (raw_convert<string> (_entry_point));
        a->add_child("Duration")->add_child_text (raw_convert<string> (_duration));
	if (_hash) {
		a->add_child("Hash")->add_child_text (_hash.get());
	}
	return a;
}

pair<string, string>
ReelAsset::cpl_node_attribute (Standard) const
{
	return make_pair ("", "");
}

pair<string, string>
ReelAsset::cpl_node_namespace (Standard) const
{
	return make_pair ("", "");
}

bool
ReelAsset::equals (shared_ptr<const ReelAsset> other, EqualityOptions opt, NoteHandler note) const
{
	if (_annotation_text != other->_annotation_text) {
		string const s = "Reel: annotation texts differ (" + _annotation_text + " vs " + other->_annotation_text + ")\n";
		if (!opt.reel_annotation_texts_can_differ) {
			note (DCP_ERROR, s);
			return false;
		} else {
			note (DCP_NOTE, s);
		}
	}

	if (_edit_rate != other->_edit_rate) {
		note (DCP_ERROR, "Reel: edit rates differ");
		return false;
	}

	if (_intrinsic_duration != other->_intrinsic_duration) {
		note (DCP_ERROR, String::compose ("Reel: intrinsic durations differ (%1 vs %2)", _intrinsic_duration, other->_intrinsic_duration));
		return false;
	}

	if (_entry_point != other->_entry_point) {
		note (DCP_ERROR, "Reel: entry points differ");
		return false;
	}

	if (_duration != other->_duration) {
		note (DCP_ERROR, "Reel: durations differ");
		return false;
	}

	if (_hash != other->_hash) {
		if (!opt.reel_hashes_can_differ) {
			note (DCP_ERROR, "Reel: hashes differ");
			return false;
		} else {
			note (DCP_NOTE, "Reel: hashes differ");
		}
	}

	if (_asset_ref.resolved () && other->_asset_ref.resolved ()) {
		return _asset_ref->equals (other->_asset_ref.asset(), opt, note);
	}

	return true;
}
