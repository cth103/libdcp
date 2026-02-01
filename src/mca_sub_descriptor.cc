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


#include "dcp_assert.h"
#include "mca_sub_descriptor.h"
#include "util.h"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS


using std::string;
using namespace dcp;


MCASubDescriptor::MCASubDescriptor(cxml::ConstNodePtr node)
{
	tag = node->name();
	instance_id = remove_urn_uuid(node->string_child("InstanceID"));
	mca_label_dictionary_id = node->string_child("MCALabelDictionaryID");
        mca_link_id = remove_urn_uuid(node->string_child("MCALinkID"));
        mca_tag_symbol = node->string_child("MCATagSymbol");
        mca_tag_name = node->optional_string_child("MCATagName");
        mca_channel_id = node->optional_string_child("MCAChannelID");
        rfc5646_spoken_language = node->optional_string_child("RFC5646SpokenLanguage");
	if (auto id = node->optional_string_child("SoundfieldGroupLinkID")) {
		soundfield_group_link_id = remove_urn_uuid(*id);
	}
}


void
MCASubDescriptor::as_xml(xmlpp::Element* parent) const
{
	auto node = cxml::add_child(parent, tag, string("r0"));
	cxml::add_child(node, "InstanceID", string("r1"))->add_child_text("urn:uuid:" + instance_id);
	cxml::add_child(node, "MCALabelDictionaryID", string("r1"))->add_child_text(mca_label_dictionary_id);
	cxml::add_child(node, "MCALinkID", string("r1"))->add_child_text("urn:uuid:" + mca_link_id);
	cxml::add_child(node, "MCATagSymbol", string("r1"))->add_child_text(mca_tag_symbol);
	if (mca_tag_name) {
		cxml::add_child(node, "MCATagName", string("r1"))->add_child_text(*mca_tag_name);
	}
	if (mca_channel_id) {
		cxml::add_child(node, "MCAChannelID", string("r1"))->add_child_text(*mca_channel_id);
	}
	if (rfc5646_spoken_language) {
		cxml::add_child(node, "RFC5646SpokenLanguage", string("r1"))->add_child_text(*rfc5646_spoken_language);
	}
	if (soundfield_group_link_id) {
		cxml::add_child(node, "SoundfieldGroupLinkID", string("r1"))->add_child_text("urn:uuid:" + *soundfield_group_link_id);
	}
}

