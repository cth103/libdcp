/*
    Copyright (C) 2016 Carl Hetherington <cth@carlh.net>

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

#include "asset_reader.h"
#include <boost/shared_ptr.hpp>

namespace ASDCP {
	namespace JP2K {
		class MXFReader;
	}
}

namespace dcp {

class MonoPictureFrame;
class MonoPictureAsset;

class MonoPictureAssetReader : public AssetReader
{
public:
	~MonoPictureAssetReader ();
	boost::shared_ptr<const MonoPictureFrame> get_frame (int n) const;

private:
	friend class MonoPictureAsset;

	MonoPictureAssetReader (MonoPictureAsset const *);

	ASDCP::JP2K::MXFReader* _reader;
};

}
