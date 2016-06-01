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
*/

/** @file  src/reel_sound_asset.h
 *  @brief ReelSoundAsset class.
 */

#include "reel_mxf.h"
#include "reel_asset.h"
#include "sound_asset.h"
#include <boost/shared_ptr.hpp>
#include <string>

namespace dcp {

/** @class ReelSoundAsset
 *  @brief Part of a Reel's description which refers to a sound asset.
 */
class ReelSoundAsset : public ReelAsset, public ReelMXF
{
public:
	ReelSoundAsset (boost::shared_ptr<dcp::SoundAsset> content, int64_t entry_point);
	ReelSoundAsset (boost::shared_ptr<const cxml::Node>);

	void write_to_cpl (xmlpp::Node* node, Standard standard) const;

	/** @return the SoundAsset that this object refers to */
	boost::shared_ptr<SoundAsset> asset () {
		return asset_of_type<SoundAsset> ();
	}

	/** @return the SoundAsset that this object refers to */
	boost::shared_ptr<const SoundAsset> asset () const {
		return asset_of_type<const SoundAsset> ();
	}

private:
	std::string key_type () const;
	std::string cpl_node_name () const;
};

}
