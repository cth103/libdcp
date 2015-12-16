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

/** @file  src/reel_subtitle_asset.h
 *  @brief ReelSubtitleAsset class.
 */

#ifndef LIBDCP_REEL_SUBTITLE_ASSET_H
#define LIBDCP_REEL_SUBTITLE_ASSET_H

#include "reel_asset.h"
#include "subtitle_asset.h"

namespace dcp {

class SubtitleAsset;

/** @class ReelSubtitleAsset
 *  @brief Part of a Reel's description which refers to a subtitle XML file.
 */
class ReelSubtitleAsset : public ReelAsset
{
public:
	ReelSubtitleAsset (boost::shared_ptr<SubtitleAsset> asset, Fraction edit_rate, int64_t instrinsic_duration, int64_t entry_point);
	ReelSubtitleAsset (boost::shared_ptr<const cxml::Node>);

	boost::shared_ptr<SubtitleAsset> asset () const {
		return boost::dynamic_pointer_cast<SubtitleAsset> (_asset_ref.asset ());
	}

private:
	std::string cpl_node_name () const;
};

}

#endif
