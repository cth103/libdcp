/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

/** @file  src/reel_mxf_asset.h
 *  @brief ReelMXFAsset
 */

#ifndef LIBDCP_REEL_MXF_ASSET_H
#define LIBDCP_REEL_MXF_ASSET_H

#include "reel_asset.h"

namespace dcp {

class MXF;

/** @class ReelMXFAsset
 *  @brief Part of a Reel's description which refers to an MXF.
 */
class ReelMXFAsset : public ReelAsset
{
public:
	ReelMXFAsset ();
	ReelMXFAsset (boost::shared_ptr<MXF> mxf, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point);
	ReelMXFAsset (boost::shared_ptr<const cxml::Node>);

	/** @return the 4-character key type for this MXF (MDIK, MDAK, etc.) */
	virtual std::string key_type () const = 0;
	
	/** @return true if a KeyId is specified for this asset, implying
	 *  that its content is encrypted.
	 */
	bool encrypted () const {
		return _key_id;
	}

	/** @return Key ID to describe the key that encrypts this asset's
	 *  content, if there is one.
	 */
	boost::optional<std::string> key_id () const {
		return _key_id;
	}

private:
	boost::optional<std::string> _key_id; ///< The &lt;KeyId&gt; from the reel's entry for this asset, if there is one
};

}

#endif
