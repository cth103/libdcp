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

/** @file  src/reel_picture_asset.h
 *  @brief ReelPictureAsset class.
 */

#ifndef LIBDCP_REEL_PICTURE_ASSET_H
#define LIBDCP_REEL_PICTURE_ASSET_H

#include "reel_mxf_asset.h"
#include "picture_mxf.h"

namespace dcp {

/** @class ReelPictureAsset
 *  @brief Part of a Reel's description which refers to a picture MXF.
 */
class ReelPictureAsset : public ReelMXFAsset
{
public:
	ReelPictureAsset ();
	ReelPictureAsset (boost::shared_ptr<PictureMXF> content, int64_t entry_point);
	ReelPictureAsset (boost::shared_ptr<const cxml::Node>);

	virtual void write_to_cpl (xmlpp::Node* node, Standard standard) const;

	boost::shared_ptr<PictureMXF> mxf () {
		return boost::dynamic_pointer_cast<PictureMXF> (_content.object ());
	}

	Fraction frame_rate () const {
		return _frame_rate;
	}

	void set_screen_aspect_ratio (Fraction a) {
		_screen_aspect_ratio = a;
	}

private:
	std::string key_type () const;
	
	Fraction _frame_rate;
	Fraction _screen_aspect_ratio;
};

}

#endif
