/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_REEL_MXF_ASSET_H
#define LIBDCP_REEL_MXF_ASSET_H

#include "reel_asset.h"

namespace dcp {

class MXF;	

class ReelMXFAsset : public ReelAsset
{
public:
	ReelMXFAsset ();
	ReelMXFAsset (boost::shared_ptr<MXF> mxf, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point);
	ReelMXFAsset (boost::shared_ptr<const cxml::Node>);

	void write_to_cpl (xmlpp::Node* node, Standard standard) const;
	
	/** @return true if a KeyId is specified for this asset, implying
	 *  that its content is encrypted.
	 */
	bool encrypted () const {
		return !_key_id.empty ();
	}

	/** @return Key ID to describe the key that encrypts this asset's;
	 *  content.
	 */
	std::string key_id () const {
		return _key_id;
	}

private:
	std::string _key_id;          ///< The &lt;KeyId&gt; from the reel's entry for this asset, or empty if there isn't one
};

}

#endif
