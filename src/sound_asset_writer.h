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

/** @file  src/sound_asset_writer.h
 *  @brief SoundAssetWriter class.
 */

#include "asset_writer.h"
#include "types.h"
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

namespace dcp {

class SoundFrame;
class SoundAsset;

/** @class SoundAssetWriter
 *  @brief A helper class for writing to SoundAssets.
 *
 *  Objects of this class can only be created with SoundAsset::start_write().
 *
 *  Sound samples can be written to the SoundAsset by calling write() with
 *  a buffer of float values.  finalize() must be called after the last samples
 *  have been written.
 */
class SoundAssetWriter : public AssetWriter
{
public:
	void write (float const * const *, int);
	bool finalize ();

private:
	friend class SoundAsset;

	SoundAssetWriter (SoundAsset *, boost::filesystem::path, Standard standard);

	void write_current_frame ();

	/* do this with an opaque pointer so we don't have to include
	   ASDCP headers
	*/

	struct ASDCPState;
	boost::shared_ptr<ASDCPState> _state;

	SoundAsset* _sound_asset;
	int _frame_buffer_offset;
};

}
