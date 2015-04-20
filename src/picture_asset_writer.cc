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

#include <inttypes.h>
#include <stdint.h>
#include "AS_DCP.h"
#include "KM_fileio.h"
#include "picture_asset_writer.h"
#include "exceptions.h"
#include "picture_asset.h"

using std::istream;
using std::ostream;
using std::string;
using boost::shared_ptr;
using namespace libdcp;

PictureAssetWriter::PictureAssetWriter (PictureAsset* asset, bool overwrite)
	: _asset (asset)
	, _frames_written (0)
	, _started (false)
	, _finalized (false)
	, _overwrite (overwrite)
{

}
