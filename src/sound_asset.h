/*
    Copyright (C) 2012-2016 Carl Hetherington <cth@carlh.net>

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

/** @file  src/sound_asset.h
 *  @brief SoundAsset class
 */

#ifndef LIBDCP_SOUND_ASSET_H
#define LIBDCP_SOUND_ASSET_H

#include "mxf.h"
#include "types.h"
#include "metadata.h"

namespace dcp
{

class SoundFrame;
class SoundAssetWriter;

/** @class SoundAsset
 *  @brief Representation of a sound asset
 */
class SoundAsset : public Asset, public MXF
{
public:
	SoundAsset (boost::filesystem::path file);
	SoundAsset (Fraction edit_rate, int sampling_rate, int channels);

	boost::shared_ptr<SoundAssetWriter> start_write (boost::filesystem::path file, Standard standard);

	bool equals (
		boost::shared_ptr<const Asset> other,
		EqualityOptions opt,
		NoteHandler note
		) const;

	boost::shared_ptr<const SoundFrame> get_frame (int n) const;

	/** @return number of channels */
	int channels () const {
		return _channels;
	}

	/** @return sampling rate in Hz */
	int sampling_rate () const {
		return _sampling_rate;
	}

	Fraction edit_rate () const {
		return _edit_rate;
	}

	int64_t intrinsic_duration () const {
		return _intrinsic_duration;
	}

	static bool valid_mxf (boost::filesystem::path);

private:
	friend class SoundAssetWriter;

	std::string pkl_type (Standard standard) const;

	Fraction _edit_rate;
	/** The total length of this content in video frames.  The amount of
	 *  content presented may be less than this.
	 */
	int64_t _intrinsic_duration;
	int _channels;      ///< number of channels
	int _sampling_rate; ///< sampling rate in Hz
};

}

#endif
