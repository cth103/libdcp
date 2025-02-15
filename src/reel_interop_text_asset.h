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


/** @file  src/reel_interop_text_asset.h
 *  @brief ReelInteropTextAsset class
 */


#include "interop_text_asset.h"
#include "reel_text_asset.h"


namespace dcp {


/** @class ReelInteropTextAsset
 *  @brief Part of a Reel's description which refers to an Interop subtitle or caption XML file
 */
class ReelInteropTextAsset : public ReelTextAsset
{
public:
	ReelInteropTextAsset(TextType type, std::shared_ptr<TextAsset> asset, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point);
	explicit ReelInteropTextAsset (std::shared_ptr<const cxml::Node>);

	std::shared_ptr<const InteropTextAsset> interop_asset() const {
		return asset_of_type<const InteropTextAsset>();
	}

	std::shared_ptr<InteropTextAsset> interop_asset() {
		return asset_of_type<InteropTextAsset>();
	}

	xmlpp::Element* write_to_cpl(xmlpp::Element* node, Standard standard) const override;

protected:
	std::string cpl_node_name() const override;
	std::pair<std::string, std::string> cpl_node_namespace() const override;
};


}

