/*
    Copyright (C) 2016 Carl Hetherington <cth@carlh.net>

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

/** @file  src/reel_atmos_asset.h
 *  @brief ReelAtmosAsset class.
 */

#ifndef LIBDCP_REEL_ATMOS_ASSET_H
#define LIBDCP_REEL_ATMOS_ASSET_H

#include "reel_asset.h"
#include "atmos_asset.h"
#include "reel_mxf.h"

namespace dcp {

class AtmosAsset;

/** @class ReelAtmosAsset
 *  @brief Part of a Reel's description which refers to a Atmos MXF.
 */
class ReelAtmosAsset : public ReelAsset, public ReelMXF
{
public:
	ReelAtmosAsset (boost::shared_ptr<AtmosAsset> asset, int64_t entry_point);
	ReelAtmosAsset (boost::shared_ptr<const cxml::Node>);

	boost::shared_ptr<AtmosAsset> asset () const {
		return asset_of_type<AtmosAsset> ();
	}

	void write_to_cpl (xmlpp::Node* node, Standard standard) const;

private:
	std::string key_type () const;
	std::string cpl_node_name () const;
};

}

#endif
