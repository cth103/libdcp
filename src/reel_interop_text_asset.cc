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


#include "dcp_assert.h"
#include "reel_interop_text_asset.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS


using std::make_pair;
using std::pair;
using std::string;
using boost::optional;
using namespace dcp;


ReelInteropTextAsset::ReelInteropTextAsset(TextType type, std::shared_ptr<TextAsset> asset, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point)
	: ReelTextAsset(type, asset, edit_rate, intrinsic_duration, entry_point)
{

}


ReelInteropTextAsset::ReelInteropTextAsset(std::shared_ptr<const cxml::Node> node)
	: ReelTextAsset(node)
{
	node->done ();
}


string
ReelInteropTextAsset::cpl_node_name() const
{
	switch (_type) {
	case TextType::OPEN_SUBTITLE:
		return "MainSubtitle";
	case TextType::CLOSED_CAPTION:
		return "cc-cpl:MainClosedCaption";
	}

	DCP_ASSERT(false);
	return "";
}


pair<string, string>
ReelInteropTextAsset::cpl_node_namespace() const
{
	switch (_type) {
	case TextType::OPEN_SUBTITLE:
		return {};
	case TextType::CLOSED_CAPTION:
		return make_pair("http://www.digicine.com/PROTO-ASDCP-CC-CPL-20070926#", "cc-cpl");
	}

	DCP_ASSERT(false);
	return {};
}


xmlpp::Element *
ReelInteropTextAsset::write_to_cpl(xmlpp::Element* node, Standard standard) const
{
	auto asset = ReelFileAsset::write_to_cpl (node, standard);
	if (_language) {
		cxml::add_text_child(asset, "Language", *_language);
	}
	return asset;
}


