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

/** @file  src/reel_sound_asset.h
 *  @brief ReelSoundAsset class.
 */

#include "reel_mxf_asset.h"
#include "sound_mxf.h"
#include <boost/shared_ptr.hpp>
#include <string>

namespace dcp {

/** @class ReelSoundAsset
 *  @brief Part of a Reel's description which refers to a sound MXF.
 */
class ReelSoundAsset : public ReelMXFAsset
{
public:
	ReelSoundAsset (boost::shared_ptr<dcp::SoundMXF> content, int64_t entry_point);
	ReelSoundAsset (boost::shared_ptr<const cxml::Node>);

	void write_to_cpl (xmlpp::Node* node, Standard standard) const;

	/** @return the SoundMXF that this object refers to */
	boost::shared_ptr<SoundMXF> mxf () {
		return boost::dynamic_pointer_cast<SoundMXF> (_asset.object ());
	}

	/** @return the SoundMXF that this object refers to */
	boost::shared_ptr<const SoundMXF> mxf () const {
		return boost::dynamic_pointer_cast<const SoundMXF> (_asset.object ());
	}
	
private:
	std::string key_type () const;
	std::string cpl_node_name () const;
};

}

