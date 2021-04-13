/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/sound_asset_writer.h
 *  @brief SoundAssetWriter class
 */


#include "asset_writer.h"
#include "fsk.h"
#include "types.h"
#include "sound_frame.h"
#include <memory>
#include <boost/filesystem.hpp>
#include <boost/shared_array.hpp>


struct sync_test1;


namespace dcp {


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
	/** @param data Pointer an array of float pointers, one for each channel.
	 *  @param frames Number of frames i.e. number of floats that are given for each channel.
	 */
	void write (float const * const *, int);

	bool finalize () override;

private:
	friend class SoundAsset;
	friend struct ::sync_test1;

	SoundAssetWriter (SoundAsset *, boost::filesystem::path, bool sync);

	void start ();
	void write_current_frame ();
	std::vector<bool> create_sync_packets ();

	/* do this with an opaque pointer so we don't have to include
	   ASDCP headers
	*/

	struct ASDCPState;
	std::shared_ptr<ASDCPState> _state;

	SoundAsset* _asset = nullptr;
	int _frame_buffer_offset = 0;

	/** true to ignore any signal passed to write() on channel 14 and instead write a sync track */
	bool _sync = false;
	/** index of the sync packet (0-3) which starts the next edit unit */
	int _sync_packet = 0;
	FSK _fsk;
};

}
