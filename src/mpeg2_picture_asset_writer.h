/*
    Copyright (C) 2024 Carl Hetherington <cth@carlh.net>

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


#ifndef LIBDCP_MPEG2_PICTURE_ASSET_WRITER_H
#define LIBDCP_MPEG2_PICTURE_ASSET_WRITER_H


#include "asset_writer.h"
#include "frame_info.h"


namespace dcp {


class Data;
class MPEG2PictureAsset;


class MPEG2PictureAssetWriter : public AssetWriter
{
public:
	virtual MPEG2FrameInfo write(uint8_t const* data, int size) = 0;
	virtual void fake_write(MPEG2FrameInfo const& info) = 0;

	MPEG2FrameInfo write(Data const& data);

protected:
	template <class P, class Q>
	friend void start(MPEG2PictureAssetWriter *, std::shared_ptr<P>, Q *, uint8_t const *, int);

	MPEG2PictureAssetWriter(MPEG2PictureAsset* asset, boost::filesystem::path file, bool overwrite);

	MPEG2PictureAsset* _picture_asset = nullptr;
	bool _overwrite = false;
};


}


#endif

