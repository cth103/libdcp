/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

/** @file  src/reel_subtitle_asset.cc
 *  @brief ReelSubtitleAsset class.
 */

#include "subtitle_asset.h"
#include "reel_subtitle_asset.h"

using std::string;
using boost::shared_ptr;
using namespace dcp;

ReelSubtitleAsset::ReelSubtitleAsset (boost::shared_ptr<SubtitleAsset> asset, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point)
	: ReelAsset (asset, edit_rate, intrinsic_duration, entry_point)
{

}

ReelSubtitleAsset::ReelSubtitleAsset (boost::shared_ptr<const cxml::Node> node)
	: ReelAsset (node)
{
	node->ignore_child ("Language");
	node->done ();
}

string
ReelSubtitleAsset::cpl_node_name () const
{
	return "MainSubtitle";
}
