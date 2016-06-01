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

#ifndef LIBDCP_MONO_PICTURE_ASSET_H
#define LIBDCP_MONO_PICTURE_ASSET_H

#include "picture_asset.h"

namespace dcp {

class MonoPictureAssetWriter;
class MonoPictureAssetReader;

/** @class MonoPictureAsset
 *  @brief A 2D (monoscopic) picture asset.
 */
class MonoPictureAsset : public PictureAsset
{
public:
	/** Create a MonoPictureAsset by reading a file.
	 *  @param file Asset file to read.
	 */
	MonoPictureAsset (boost::filesystem::path file);

	/** Create a MonoPictureAsset with a given edit rate.
	 *  @param edit_rate Edit rate (i.e. frame rate) in frames per second.
	 */
	MonoPictureAsset (Fraction edit_rate);

	/** Start a progressive write to a MonoPictureAsset */
	boost::shared_ptr<PictureAssetWriter> start_write (boost::filesystem::path, Standard standard, bool);
	boost::shared_ptr<MonoPictureAssetReader> start_read () const;

	bool equals (
		boost::shared_ptr<const Asset> other,
		EqualityOptions opt,
		NoteHandler note
		) const;

private:
	std::string cpl_node_name () const;
};

}

#endif
