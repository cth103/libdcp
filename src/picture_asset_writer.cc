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

#include "KM_fileio.h"
#include "picture_asset_writer.h"
#include "exceptions.h"
#include "picture_asset.h"
#include "AS_DCP.h"
#include <inttypes.h>
#include <stdint.h>

using std::istream;
using std::ostream;
using std::string;
using boost::shared_ptr;
using namespace dcp;

PictureAssetWriter::PictureAssetWriter (PictureAsset* asset, boost::filesystem::path file, Standard standard, bool overwrite)
	: AssetWriter (asset, file, standard)
	, _picture_asset (asset)
	, _standard (standard)
	, _overwrite (overwrite)
{
	asset->set_file (file);
}
