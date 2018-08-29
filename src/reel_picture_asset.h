/*
    Copyright (C) 2014-2015 Carl Hetherington <cth@carlh.net>

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

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
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
	explicit ReelPictureAsset (boost::shared_ptr<const cxml::Node>);

	virtual xmlpp::Node* write_to_cpl (xmlpp::Node* node, Standard standard) const;
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
