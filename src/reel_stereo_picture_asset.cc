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

#include "reel_stereo_picture_asset.h"

using std::string;
using std::pair;
using std::make_pair;
using boost::shared_ptr;
using namespace dcp;

ReelStereoPictureAsset::ReelStereoPictureAsset (shared_ptr<const cxml::Node> node)
	: ReelPictureAsset (node)
{

}

string
ReelStereoPictureAsset::cpl_node_name () const
{
	return "msp-cpl:MainStereoscopicPicture";
}

pair<string, string>
ReelStereoPictureAsset::cpl_node_attribute (Standard standard) const
{
	if (standard == INTEROP) {
		return make_pair ("xmlns:msp-cpl", "http://www.digicine.com/schemas/437-Y/2007/Main-Stereo-Picture-CPL");
	} else {
		return make_pair ("xmlns:msp-cpl", "http://www.smpte-ra.org/schemas/429-10/2008/Main-Stereo-Picture-CPL");
	}

	return make_pair ("", "");
}
