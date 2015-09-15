/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

/** @file  src/picture_asset_writer.h
 *  @brief PictureAssetWriter and FrameInfo classes.
 */

#ifndef LIBDCP_PICTURE_ASSET_WRITER_H
#define LIBDCP_PICTURE_ASSET_WRITER_H

#include "metadata.h"
#include "types.h"
#include "asset_writer.h"
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <stdint.h>
#include <string>
#include <fstream>

namespace dcp {

class PictureAsset;

/** @class FrameInfo
 *  @brief Information about a single frame (either a monoscopic frame or a left *or* right eye stereoscopic frame)
 */
struct FrameInfo
{
	FrameInfo ()
		: offset (0)
		, size (0)
	{}

	FrameInfo (uint64_t o, uint64_t s, std::string h)
		: offset (o)
		, size (s)
		, hash (h)
	{}

	uint64_t offset;
	uint64_t size;
	std::string hash;
};

/** @class PictureAssetWriter
 *  @brief Parent class for classes which write picture assets.
 */
class PictureAssetWriter : public AssetWriter
{
public:
	virtual FrameInfo write (uint8_t *, int) = 0;
	virtual void fake_write (int) = 0;

protected:
	template <class P, class Q>
	friend void start (PictureAssetWriter *, boost::shared_ptr<P>, Standard, Q *, uint8_t *, int);

	PictureAssetWriter (PictureAsset *, boost::filesystem::path, Standard standard, bool);

	PictureAsset* _picture_asset;
	Standard _standard;
	bool _overwrite;
};

}

#endif
