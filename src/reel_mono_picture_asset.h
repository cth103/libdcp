/*
    Copyright (C) 2014-2015 Carl Hetherington <cth@carlh.net>

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

/** @file  src/reel_mono_picture_asset.h
 *  @brief ReelMonoPictureAsset class.
 */

#ifndef LIBDCP_REEL_MONO_PICTURE_ASSET_H
#define LIBDCP_REEL_MONO_PICTURE_ASSET_H

#include "reel_picture_asset.h"
#include "mono_picture_asset.h"

namespace dcp {

class MonoPictureAsset;

/** @class ReelMonoPictureAsset
 *  @brief Part of a Reel's description which refers to a monoscopic picture asset.
 */
class ReelMonoPictureAsset : public ReelPictureAsset
{
public:
	ReelMonoPictureAsset ();
	ReelMonoPictureAsset (boost::shared_ptr<MonoPictureAsset> asset, int64_t entry_point);
	ReelMonoPictureAsset (boost::shared_ptr<const cxml::Node>);

	/** @return the MonoPictureAsset that this object refers to */
	boost::shared_ptr<const MonoPictureAsset> mono_asset () const {
		return boost::dynamic_pointer_cast<const MonoPictureAsset> (_asset_ref.asset ());
	}

	/** @return the MonoPictureAsset that this object refers to */
	boost::shared_ptr<MonoPictureAsset> mono_asset () {
		return boost::dynamic_pointer_cast<MonoPictureAsset> (_asset_ref.asset ());
	}

private:
	std::string cpl_node_name () const;
};

}

#endif
