/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_STEREO_PICTURE_ASSET_H
#define LIBDCP_STEREO_PICTURE_ASSET_H

#include "picture_asset.h"

namespace dcp {

class StereoPictureAssetReader;

/** A 3D (stereoscopic) picture asset */
class StereoPictureAsset : public PictureAsset
{
public:
	explicit StereoPictureAsset (boost::filesystem::path file);
	explicit StereoPictureAsset (Fraction edit_rate);

	/** Start a progressive write to a StereoPictureAsset */
	boost::shared_ptr<PictureAssetWriter> start_write (boost::filesystem::path file, Standard, bool);
	boost::shared_ptr<StereoPictureAssetReader> start_read () const;

	bool equals (
		boost::shared_ptr<const Asset> other,
		EqualityOptions opt,
		NoteHandler note
		) const;
};

}

#endif
