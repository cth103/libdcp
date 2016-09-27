/*
    Copyright (C) 2012-2016 Carl Hetherington <cth@carlh.net>

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

/** @file  src/sound_asset.h
 *  @brief SoundAsset class
 */

#ifndef LIBDCP_SOUND_ASSET_H
#define LIBDCP_SOUND_ASSET_H

#include "mxf.h"
#include "types.h"
#include "metadata.h"
#include "sound_frame.h"

namespace dcp
{

class SoundAssetWriter;
class SoundAssetReader;

/** @class SoundAsset
 *  @brief Representation of a sound asset
 */
class SoundAsset : public Asset, public MXF
{
public:
	explicit SoundAsset (boost::filesystem::path file);
	SoundAsset (Fraction edit_rate, int sampling_rate, int channels);

	boost::shared_ptr<SoundAssetWriter> start_write (boost::filesystem::path file, Standard standard);
	boost::shared_ptr<SoundAssetReader> start_read () const;

	bool equals (
		boost::shared_ptr<const Asset> other,
		EqualityOptions opt,
		NoteHandler note
		) const;

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
