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

/** @file  src/reel_picture_asset.h
 *  @brief ReelPictureAsset class.
 */

#ifndef LIBDCP_REEL_PICTURE_ASSET_H
#define LIBDCP_REEL_PICTURE_ASSET_H

#include "reel_mxf.h"
#include "reel_asset.h"
#include "picture_asset.h"

namespace dcp {

/** @class ReelPictureAsset
 *  @brief Part of a Reel's description which refers to a picture asset.
 */
class ReelPictureAsset : public ReelAsset, public ReelMXF
{
public:
	ReelPictureAsset ();
	ReelPictureAsset (boost::shared_ptr<PictureAsset> asset, int64_t entry_point);
	ReelPictureAsset (boost::shared_ptr<const cxml::Node>);

	virtual void write_to_cpl (xmlpp::Node* node, Standard standard) const;
	virtual bool equals (boost::shared_ptr<const ReelAsset>, EqualityOptions, NoteHandler) const;

	/** @return the PictureAsset that this object refers to */
	boost::shared_ptr<const PictureAsset> asset () const {
		return asset_of_type<const PictureAsset> ();
	}

	/** @return the PictureAsset that this object refers to */
	boost::shared_ptr<PictureAsset> asset () {
		return asset_of_type<PictureAsset> ();
	}

	/** @return picture frame rate */
	Fraction frame_rate () const {
		return _frame_rate;
	}

	/** Set the ScreenAspectRatio of this asset.
	 *  @param a New aspect ratio.
	 */
	void set_screen_aspect_ratio (Fraction a) {
		_screen_aspect_ratio = a;
	}

	Fraction screen_aspect_ratio () const {
		return _screen_aspect_ratio;
	}

private:
	std::string key_type () const;

	Fraction _frame_rate;
	Fraction _screen_aspect_ratio;
};

}

#endif
