/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#include "raw_convert.h"
#include "reel_asset.h"
#include "content.h"
#include "compose.hpp"
#include <libcxml/cxml.h>

using std::pair;
using std::string;
using std::make_pair;
using boost::shared_ptr;
using namespace dcp;

ReelAsset::ReelAsset ()
	: Object (make_uuid ())
	, _content (_id)
	, _edit_rate (Fraction (24, 1))
	, _intrinsic_duration (0)
	, _entry_point (0)
	, _duration (0)
{

}

/** Construct a ReelAsset.
 *  @param content Content that this asset refers to.
 *  @param entry_point Entry point to use in that content.
 */
ReelAsset::ReelAsset (boost::shared_ptr<Content> content, int64_t entry_point)
	: Object (content->id ())
	, _content (content)
	, _edit_rate (content->edit_rate ())
	, _intrinsic_duration (content->intrinsic_duration ())
	, _entry_point (entry_point)
	, _duration (_intrinsic_duration - _entry_point)
	, _hash (make_digest (content->file (), 0))
{
	/* default _annotation_text to the leaf name of our file */
        _annotation_text = content->file().leaf().string ();
}

ReelAsset::ReelAsset (boost::shared_ptr<const cxml::Node> node)
	: Object (node->string_child ("Id"))
	, _content (_id)
	, _annotation_text (node->optional_string_child ("AnnotationText").get_value_or (""))
	, _edit_rate (Fraction (node->string_child ("EditRate")))
	, _intrinsic_duration (node->number_child<int64_t> ("IntrinsicDuration"))
	, _entry_point (node->number_child<int64_t> ("EntryPoint"))
	, _duration (node->number_child<int64_t> ("Duration"))
	, _hash (node->optional_string_child ("Hash").get_value_or (""))
	, _key_id (node->optional_string_child ("KeyId").get_value_or (""))
{
	if (_id.length() > 9) {
		_id = _id.substr (9);
		_content.set_id (_id);
	}
	
	if (_key_id.length() > 9) {
		_key_id = _key_id.substr (9);
	}
}

void
ReelAsset::write_to_cpl (xmlpp::Node* node, Standard) const
{
        pair<string, string> const attr = cpl_node_attribute ();
        xmlpp::Element* a = node->add_child (cpl_node_name ());
        if (!attr.first.empty ()) {
                a->set_attribute (attr.first, attr.second);
        }
        a->add_child("Id")->add_child_text ("urn:uuid:" + _id);
        a->add_child("AnnotationText")->add_child_text (_annotation_text);
        a->add_child("EditRate")->add_child_text (String::compose ("%1 %2", _edit_rate.numerator, _edit_rate.denominator));
        a->add_child("IntrinsicDuration")->add_child_text (raw_convert<string> (_intrinsic_duration));
        a->add_child("EntryPoint")->add_child_text (raw_convert<string> (_entry_point));
        a->add_child("Duration")->add_child_text (raw_convert<string> (_duration));
        if (!_key_id.empty ()) {
                a->add_child("KeyId")->add_child_text ("urn:uuid:" + _key_id);
        }
}

pair<string, string>
ReelAsset::cpl_node_attribute () const
{
	return make_pair ("", "");
}
