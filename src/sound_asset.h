/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_SOUND_ASSET_H
#define LIBDCP_SOUND_ASSET_H

/** @file  src/sound_asset.h
 *  @brief An asset made up of PCM audio data files
 */

#include "mxf_asset.h"
#include "types.h"
#include "metadata.h"

namespace libdcp
{

class SoundFrame;
class SoundAsset;

class SoundAssetWriter
{
public:
	void write (float const * const *, int);
	void finalize ();

private:
	friend class SoundAsset;

	SoundAssetWriter (SoundAsset *);

	/* no copy construction */
	SoundAssetWriter (SoundAssetWriter const &);
	SoundAssetWriter& operator= (SoundAssetWriter const &);
	
	void write_current_frame ();

	/* do this with an opaque pointer so we don't have to include
	   ASDCP headers
	*/
	   
	struct ASDCPState;
	boost::shared_ptr<ASDCPState> _state;

	SoundAsset* _asset;
	bool _finalized;
	int _frames_written;
	int _frame_buffer_offset;
};

/** @brief An asset made up of WAV files */
class SoundAsset : public MXFAsset
{
public:
	SoundAsset (boost::filesystem::path directory, boost::filesystem::path mxf_name);

	void read ();

	/** The following parameters must be set up (if required) before calling this:
	 *      Interop mode (set_interop)
	 *      Edit rate    (set_edit_rate)
	 *      MXF Metadata (set_metadata)
	 *      Channels     (set_channels)
	 *      Intrinsic duration (set_intrinsic_duration)
	 */
	void create (std::vector<boost::filesystem::path> const & files);

	/** The following parameters must be set up (if required) before calling this:
	 *      Interop mode (set_interop)
	 *      Edit rate    (set_edit_rate)
	 *      MXF Metadata (set_metadata)
	 *      Channels     (set_channels)
	 *      Intrinsic duration (set_intrinsic_duration)
	 */
	void create (boost::function<boost::filesystem::path (Channel)> get_path);

	boost::shared_ptr<SoundAssetWriter> start_write ();
	
	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;

	boost::shared_ptr<const SoundFrame> get_frame (int n) const;

	void set_channels (int c) {
		_channels = c;
	}
	
	int channels () const {
		return _channels;
	}

	void set_sampling_rate (int s) {
		_sampling_rate = s;
	}

	int sampling_rate () const {
		return _sampling_rate;
	}

protected:

	std::string asdcp_kind () const {
		return "Sound";
	}
	
private:
	std::string key_type () const;
	void construct (boost::function<boost::filesystem::path (Channel)> get_path);
	boost::filesystem::path path_from_channel (Channel channel, std::vector<boost::filesystem::path> const & files);
	std::string cpl_node_name () const;

	/** Number of channels in the asset */
	int _channels;
	int _sampling_rate;
};

}

#endif
