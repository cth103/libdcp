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

#include "mono_picture_asset_reader.h"
#include "mono_picture_asset.h"
#include "mono_picture_frame.h"
#include "exceptions.h"
#include "AS_DCP.h"

using namespace dcp;
using boost::shared_ptr;

MonoPictureAssetReader::MonoPictureAssetReader (MonoPictureAsset const * asset)
	: AssetReader (asset)
{
	_reader = new ASDCP::JP2K::MXFReader ();
	Kumu::Result_t const r = _reader->OpenRead (asset->file().string().c_str());
	if (ASDCP_FAILURE (r)) {
		delete _reader;
		boost::throw_exception (FileError ("could not open MXF file for reading", asset->file(), r));
	}
}

MonoPictureAssetReader::~MonoPictureAssetReader ()
{
	delete _reader;
}

shared_ptr<const MonoPictureFrame>
MonoPictureAssetReader::get_frame (int n) const
{
	return shared_ptr<const MonoPictureFrame> (new MonoPictureFrame (_reader, n, _decryption_context));
}
