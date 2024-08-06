/*
    Copyright (C) 2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/reel_interop_text_asset.cc
 *  @brief ReelInteropTextAsset class
 */


#include "reel_smpte_text_asset.h"
#include "smpte_text_asset.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS


using std::make_pair;
using std::pair;
using std::shared_ptr;
using std::string;
using boost::optional;
using namespace dcp;


ReelSMPTETextAsset::ReelSMPTETextAsset(TextType type, shared_ptr<SMPTETextAsset> asset, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point)
	: ReelTextAsset(type, asset, edit_rate, intrinsic_duration, entry_point)
{

}


ReelSMPTETextAsset::ReelSMPTETextAsset(shared_ptr<const cxml::Node> node)
	: ReelTextAsset(node)
{
	node->done ();
}



string
ReelSMPTETextAsset::cpl_node_name() const
{
	switch (_type) {
	case TextType::OPEN_SUBTITLE:
		return "MainSubtitle";
	case TextType::OPEN_CAPTION:
		return "tt:MainCaption";
	case TextType::CLOSED_SUBTITLE:
		return "ClosedSubtitle";
	case TextType::CLOSED_CAPTION:
		return "tt:ClosedCaption";
	}

	DCP_ASSERT(false);
	return "";
}


pair<string, string>
ReelSMPTETextAsset::cpl_node_namespace() const
{
	switch (_type) {
	case TextType::OPEN_SUBTITLE:
	case TextType::CLOSED_SUBTITLE:
		return {};
	case TextType::OPEN_CAPTION:
	case TextType::CLOSED_CAPTION:
		return make_pair("http://www.smpte-ra.org/schemas/429-12/2008/TT", "tt");
	}

	DCP_ASSERT(false);
	return {};
}


xmlpp::Element *
ReelSMPTETextAsset::write_to_cpl(xmlpp::Element* node, Standard standard) const
{
	auto asset = ReelFileAsset::write_to_cpl (node, standard);
	string const ns = _type == TextType::CLOSED_CAPTION ? "tt" : "";
	if (_language) {
		cxml::add_child(asset, "Language", ns)->add_child_text(*_language);
	}
	return asset;
}


