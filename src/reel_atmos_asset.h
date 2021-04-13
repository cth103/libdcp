/*
    Copyright (C) 2016-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/reel_atmos_asset.h
 *  @brief ReelAtmosAsset class
 */


#ifndef LIBDCP_REEL_ATMOS_ASSET_H
#define LIBDCP_REEL_ATMOS_ASSET_H


#include "reel_file_asset.h"
#include "atmos_asset.h"


namespace dcp {


class AtmosAsset;


/** @class ReelAtmosAsset
 *  @brief Part of a Reel's description which refers to a Atmos MXF
 */
class ReelAtmosAsset : public ReelFileAsset
{
public:
	ReelAtmosAsset (std::shared_ptr<AtmosAsset> asset, int64_t entry_point);
	explicit ReelAtmosAsset (std::shared_ptr<const cxml::Node>);

	std::shared_ptr<const AtmosAsset> asset () const {
		return asset_of_type<const AtmosAsset>();
	}

	std::shared_ptr<AtmosAsset> asset () {
		return asset_of_type<AtmosAsset>();
	}

	xmlpp::Node* write_to_cpl (xmlpp::Node* node, Standard standard) const override;
	bool equals (std::shared_ptr<const ReelAtmosAsset>, EqualityOptions, NoteHandler) const;

private:
	boost::optional<std::string> key_type () const override {
		return std::string("MDEK");
	}

	std::string cpl_node_name (Standard standard) const override;
	std::pair<std::string, std::string> cpl_node_namespace () const override;
};


}


#endif
