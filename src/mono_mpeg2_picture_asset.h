/*
    Copyright (C) 2023 Carl Hetherington <cth@carlh.net>

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


#ifndef LIBDCP_MONO_MPEG2_PICTURE_ASSET_H
#define LIBDCP_MONO_MPEG2_PICTURE_ASSET_H


/** @file  src/mono_mpeg2_picture_asset.h
 *  @brief MonoMPEG2PictureAsset class
 */


#include "behaviour.h"
#include "mpeg2_picture_asset.h"
#include "mono_mpeg2_picture_asset_reader.h"


namespace dcp {


class MonoMPEG2PictureAssetWriter;


class MonoMPEG2PictureAsset : public MPEG2PictureAsset
{
public:
	MonoMPEG2PictureAsset(Fraction edit_rate)
		: MPEG2PictureAsset(edit_rate)
	{}

	explicit MonoMPEG2PictureAsset(boost::filesystem::path file);

	bool can_be_read() const override;

	std::shared_ptr<MPEG2PictureAssetWriter> start_write(boost::filesystem::path file, Behaviour behaviour) override;
	std::shared_ptr<MonoMPEG2PictureAssetReader> start_read() const;
};



}


#endif
