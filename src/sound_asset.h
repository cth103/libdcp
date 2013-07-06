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

	SoundAssetWriter (SoundAsset *, MXFMetadata const &);

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
	MXFMetadata _metadata;
};

/** @brief An asset made up of WAV files */
class SoundAsset : public MXFAsset
{
public:
	/** Construct a SoundAsset, generating the MXF from some WAV files.
	 *  This may take some time; progress is indicated by emission of the Progress signal.
	 *  @param files Pathnames of sound files, in the order Left, Right, Centre, Lfe (sub), Left surround, Right surround.
	 *  @param directory Directory in which to create MXF file.
	 *  @param mxf_name Name of MXF file to create.
	 *  @param progress Signal to inform of progress.
	 *  @param fps Frames per second.
	 *  @param length Length in frames.
	 *  @param start_frame Frame in the source to start writing from.
	 *  @param intrinsic_duration Length of the whole asset in frames.
	 *  @param encrypted true if asset should be encrypted.
	 *  Note that this is different to entry_point in that the asset will contain no data before start_frame.
	 */
	SoundAsset (
		std::vector<std::string> const & files,
		std::string directory,
		std::string mxf_name,
		boost::signals2::signal<void (float)>* progress,
		int fps,
		int intrinsic_duration,
		bool encrypted,
		MXFMetadata const & metadata = MXFMetadata ()
		);

	/** Construct a SoundAsset, generating the MXF from some WAV files.
	 *  This may take some time; progress is indicated by emission of the Progress signal.
	 *  @param get_path Functor which returns a WAV file path for a given channel.
	 *  @param directory Directory in which to create MXF file.
	 *  @param mxf_name Name of MXF file to create.
	 *  @param progress Signal to inform of progress.
	 *  @param fps Frames per second.
	 *  @param intrinsic_duration Length of the whole asset in frames.
	 *  @param channels Number of audio channels.
	 *  @param encrypted true if asset should be encrypted.
	 */
	SoundAsset (
		boost::function<std::string (Channel)> get_path,
		std::string directory,
		std::string mxf_name,
		boost::signals2::signal<void (float)>* progress,
		int fps,
		int intrinsic_duration,
		int channels,
		bool encrypted,
		MXFMetadata const & metadata = MXFMetadata ()
		);

	SoundAsset (
		std::string directory,
		std::string mxf_name
		);

	SoundAsset (
		std::string directory,
		std::string mxf_name,
		int fps,
		int channels,
		int sampling_rate
		);

	boost::shared_ptr<SoundAssetWriter> start_write (MXFMetadata const & metadata = MXFMetadata ());
	

	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;

	boost::shared_ptr<const SoundFrame> get_frame (int n) const;
	
	int channels () const {
		return _channels;
	}

	int sampling_rate () const {
		return _sampling_rate;
	}

private:
	std::string key_type () const;
	void construct (boost::function<std::string (Channel)> get_path, MXFMetadata const &);
	std::string path_from_channel (Channel channel, std::vector<std::string> const & files);
	std::string cpl_node_name () const;

	/** Number of channels in the asset */
	int _channels;
	int _sampling_rate;
};

}

#endif
