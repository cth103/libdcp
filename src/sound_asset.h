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


/** @file  src/sound_asset.h
 *  @brief SoundAsset class
 */


#ifndef LIBDCP_SOUND_ASSET_H
#define LIBDCP_SOUND_ASSET_H


#include "mxf.h"
#include "language_tag.h"
#include "metadata.h"
#include "sound_frame.h"
#include "sound_asset_reader.h"


namespace dcp {
	class SoundAsset;
}


extern std::shared_ptr<dcp::SoundAsset> simple_sound (
	boost::filesystem::path path, std::string suffix, dcp::MXFMetadata mxf_meta, std::string language, int frames, int sample_rate, boost::optional<dcp::Key>, int channels
	);


namespace dcp
{


class SoundAssetWriter;


/** @class SoundAsset
 *  @brief Representation of a sound asset
 */
class SoundAsset : public Asset, public MXF
{
public:
	explicit SoundAsset (boost::filesystem::path file);
	SoundAsset (Fraction edit_rate, int sampling_rate, int channels, LanguageTag language, Standard standard);

	enum class AtmosSync {
		ENABLED,
		DISABLED
	};

	enum class MCASubDescriptors {
		ENABLED,
		DISABLED
	};

	/** @param extra_active_channels list of channels that are active in the asset, other than the basic 5.1
	 *  which are assumed always to be active.
	 */
	std::shared_ptr<SoundAssetWriter> start_write(
		boost::filesystem::path file,
		std::vector<dcp::Channel> extra_active_channels,
		AtmosSync atmos_sync,
		MCASubDescriptors mca_subdescriptors
		);

	std::shared_ptr<SoundAssetReader> start_read () const;

	bool equals (
		std::shared_ptr<const Asset> other,
		EqualityOptions const& opt,
		NoteHandler note
		) const override;

	/** @return number of channels in the MXF */
	int channels () const {
		return _channels;
	}

	/** @return An estimate of the number of channels that are actually in use */
	int active_channels() const;

	/** @return sampling rate in Hz */
	int sampling_rate () const {
		return _sampling_rate;
	}

	/** @return Bit depth of samples.  This should always be 24, but we return it
	 *  so the verification code can check
	 */
	int bit_depth() const {
		return _bit_depth;
	}

	Fraction edit_rate () const {
		return _edit_rate;
	}

	int64_t intrinsic_duration () const {
		return _intrinsic_duration;
	}

	boost::optional<std::string> language () const {
		return _language;
	}

	static bool valid_mxf (boost::filesystem::path);
	static std::string static_pkl_type (Standard standard);

private:
	friend class SoundAssetWriter;
	friend std::shared_ptr<dcp::SoundAsset> (::simple_sound) (
		boost::filesystem::path path, std::string suffix, dcp::MXFMetadata mxf_meta, std::string language, int frames, int sample_rate, boost::optional<dcp::Key>, int channels
		);

	std::string pkl_type (Standard standard) const override {
		return static_pkl_type (standard);
	}

	Fraction _edit_rate;
	/** The total length of this content in video frames.  The amount of
	 *  content presented may be less than this.
	 */
	int64_t _intrinsic_duration = 0;
	int _channels = 0;                     ///< number of channels in the MXF
	boost::optional<int> _active_channels; ///< estimate of the number of active channels
	int _sampling_rate = 0;                ///< sampling rate in Hz
	int _bit_depth = 24;
	boost::optional<std::string> _language;
};


}


#endif
