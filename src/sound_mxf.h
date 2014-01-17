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

#include "mxf.h"
#include "types.h"
#include "metadata.h"

namespace dcp
{

class SoundFrame;
class SoundMXF;

class SoundMXFWriter
{
public:
	void write (float const * const *, int);
	void finalize ();

private:
	friend class SoundMXF;

	SoundMXFWriter (SoundMXF *);

	/* no copy construction */
	SoundMXFWriter (SoundMXFWriter const &);
	SoundMXFWriter& operator= (SoundMXFWriter const &);
	
	void write_current_frame ();

	/* do this with an opaque pointer so we don't have to include
	   ASDCP headers
	*/
	   
	struct ASDCPState;
	boost::shared_ptr<ASDCPState> _state;

	SoundMXF* _asset;
	bool _finalized;
	int _frames_written;
	int _frame_buffer_offset;
};

/** @brief An asset made up of WAV files */
class SoundMXF : public MXF
{
public:
	SoundMXF (boost::filesystem::path directory, boost::filesystem::path mxf_name);

	void read ();

	boost::shared_ptr<SoundMXFWriter> start_write ();
	
	bool equals (boost::shared_ptr<const ContentAsset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;

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

private:
	std::string key_type () const;
	void construct (boost::function<boost::filesystem::path (Channel)> get_path);
	std::string cpl_node_name () const;

	/** Number of channels in the asset */
	int _channels;
	int _sampling_rate;
};

}

#endif
