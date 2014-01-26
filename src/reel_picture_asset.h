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

#ifndef LIBDCP_REEL_PICTURE_ASSET_H
#define LIBDCP_REEL_PICTURE_ASSET_H

#include "reel_asset.h"
#include "picture_mxf.h"

namespace dcp {

class ReelPictureAsset : public ReelAsset
{
public:
	ReelPictureAsset ();
	ReelPictureAsset (boost::shared_ptr<PictureMXF> content, int64_t entry_point);
	ReelPictureAsset (boost::shared_ptr<const cxml::Node>);

	boost::shared_ptr<PictureMXF> mxf () {
		return boost::dynamic_pointer_cast<PictureMXF> (_content.object ());
	}

	void set_screen_aspect_ratio (Fraction a) {
		_screen_aspect_ratio = a;
	}

	virtual void write_to_cpl (xmlpp::Node* node, Standard standard) const;

private:
	Fraction _frame_rate;
	Fraction _screen_aspect_ratio;
};

}

#endif
