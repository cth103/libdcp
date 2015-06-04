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

/** @file  src/reel_mono_picture_asset.cc
 *  @brief ReelMonoPictureAsset class.
 */

#include "reel_mono_picture_asset.h"
#include "mono_picture_asset.h"
#include <libcxml/cxml.h>

using std::string;
using boost::shared_ptr;
using namespace dcp;

ReelMonoPictureAsset::ReelMonoPictureAsset ()
{

}

ReelMonoPictureAsset::ReelMonoPictureAsset (boost::shared_ptr<MonoPictureAsset> asset, int64_t entry_point)
	: ReelPictureAsset (asset, entry_point)
{

}

ReelMonoPictureAsset::ReelMonoPictureAsset (boost::shared_ptr<const cxml::Node> node)
	: ReelPictureAsset (node)
{
	node->done ();
}

string
ReelMonoPictureAsset::cpl_node_name () const
{
	return "MainPicture";
}

