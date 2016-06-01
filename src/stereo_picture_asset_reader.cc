/*
    Copyright (C) 2016 Carl Hetherington <cth@carlh.net>

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

#include "stereo_picture_asset_reader.h"
#include "stereo_picture_asset.h"
#include "stereo_picture_frame.h"
#include "exceptions.h"
#include "AS_DCP.h"

using namespace dcp;
using boost::shared_ptr;

StereoPictureAssetReader::StereoPictureAssetReader (StereoPictureAsset const * asset)
	: AssetReader (asset)
{
	_reader = new ASDCP::JP2K::MXFSReader ();
	Kumu::Result_t const r = _reader->OpenRead (asset->file().string().c_str());
	if (ASDCP_FAILURE (r)) {
		delete _reader;
		boost::throw_exception (FileError ("could not open MXF file for reading", asset->file(), r));
	}
}

StereoPictureAssetReader::~StereoPictureAssetReader ()
{
	delete _reader;
}

shared_ptr<const StereoPictureFrame>
StereoPictureAssetReader::get_frame (int n) const
{
	return shared_ptr<const StereoPictureFrame> (new StereoPictureFrame (_reader, n, _decryption_context));
}
