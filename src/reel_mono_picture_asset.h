/*
    Copyright (C) 2014-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/reel_mono_picture_asset.h
 *  @brief ReelMonoPictureAsset class
 */


#ifndef LIBDCP_REEL_MONO_PICTURE_ASSET_H
#define LIBDCP_REEL_MONO_PICTURE_ASSET_H


#include "reel_picture_asset.h"
#include "mono_j2k_picture_asset.h"
#include "mono_mpeg2_picture_asset.h"


namespace dcp {


class MonoJ2KPictureAsset;


/** @class ReelMonoPictureAsset
 *  @brief Part of a Reel's description which refers to a monoscopic picture asset
 */
class ReelMonoPictureAsset : public ReelPictureAsset
{
public:
	ReelMonoPictureAsset(std::shared_ptr<PictureAsset> asset, int64_t entry_point);
	explicit ReelMonoPictureAsset (std::shared_ptr<const cxml::Node>);

	/** @return the MonoJ2KPictureAsset that this object refers to, if applicable */
	std::shared_ptr<const MonoJ2KPictureAsset> mono_j2k_asset() const {
		return asset_of_type<const MonoJ2KPictureAsset>();
	}

	/** @return the MonoJ2KPictureAsset that this object refers to */
	std::shared_ptr<MonoJ2KPictureAsset> mono_j2k_asset() {
		return asset_of_type<MonoJ2KPictureAsset>();
	}

	/** @return the MonoMPEG2PictureAsset that this object refers to, if applicable */
	std::shared_ptr<const MonoMPEG2PictureAsset> mono_mpeg2_asset() const {
		return asset_of_type<const MonoMPEG2PictureAsset>();
	}

	/** @return the MonoMPEG2PictureAsset that this object refers to */
	std::shared_ptr<MonoMPEG2PictureAsset> mono_mpeg2_asset() {
		return asset_of_type<MonoMPEG2PictureAsset>();
	}

private:
	std::string cpl_node_name() const override;
};


}


#endif
