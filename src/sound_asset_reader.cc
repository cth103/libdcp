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

#include "sound_asset_reader.h"
#include "sound_asset.h"
#include "sound_frame.h"
#include "exceptions.h"
#include "AS_DCP.h"

using boost::shared_ptr;
using namespace dcp;

SoundAssetReader::SoundAssetReader (SoundAsset const * asset)
	: AssetReader (asset)
{
	_reader = new ASDCP::PCM::MXFReader ();
	Kumu::Result_t const r = _reader->OpenRead (asset->file().string().c_str());
	if (ASDCP_FAILURE (r)) {
		delete _reader;
		boost::throw_exception (FileError ("could not open MXF file for reading", asset->file(), r));
	}
}

SoundAssetReader::~SoundAssetReader ()
{
	delete _reader;
}

shared_ptr<const SoundFrame>
SoundAssetReader::get_frame (int n) const
{
	return shared_ptr<const SoundFrame> (new SoundFrame (_reader, n, _decryption_context));
}
