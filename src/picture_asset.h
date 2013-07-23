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

#ifndef LIBDCP_PICTURE_ASSET_H
#define LIBDCP_PICTURE_ASSET_H

/** @file  src/picture_asset.h
 *  @brief An asset made up of JPEG2000 data
 */

#include <openjpeg.h>
#include "mxf_asset.h"
#include "util.h"
#include "metadata.h"

namespace libdcp
{

class MonoPictureFrame;	
class StereoPictureFrame;
class PictureAssetWriter;

/** @brief An asset made up of JPEG2000 data */
class PictureAsset : public MXFAsset
{
public:
	/** Construct a PictureAsset.
	 *  This class will not write anything to disk in this constructor, but subclasses may.
	 *  
	 *  @param directory Directory where MXF file is.
	 *  @param mxf_name Name of MXF file.
	 */
	PictureAsset (std::string directory, std::string mxf_name);

	/** Construct a PictureAsset.
	 *  This class will not write anything to disk in this constructor, but subclasses may.
	 *
	 *  @param directory Directory where MXF file is.
	 *  @param mxf_name Name of MXF file.
	 *  @param progress Signal to use to inform of progres, or 0.
	 *  @param fps Video frames per second.
	 *  @param intrinsic_duration Total number of frames in the asset.
	 *  @param size Size of video frame images in pixels.
	 */
	PictureAsset (
		std::string directory,
		std::string mxf_name,
		boost::signals2::signal<void (float)>* progress,
		int fps,
		int intrinsic_duration,
		bool encrypted,
		Size
		);

	/** Start a progressive write to this asset.
	 *  @param overwrite true to overwrite an existing MXF file; in this mode, writing can be resumed to a partially-written MXF; false if the
	 *  MXF file does not exist.
	 *  @param metadata MXF metadata to use.
	 */
	virtual boost::shared_ptr<PictureAssetWriter> start_write (bool overwrite, MXFMetadata const & metadata = MXFMetadata ()) = 0;
	
	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;

	Size size () const {
		return _size;
	}

	void write_to_cpl (xmlpp::Node *) const;

protected:	

	bool frame_buffer_equals (
		int frame, EqualityOptions opt, boost::function<void (NoteType, std::string)> note,
		uint8_t const * data_A, unsigned int size_A, uint8_t const * data_B, unsigned int size_B
		) const;

	/** picture size in pixels */
	Size _size;

private:
	std::string key_type () const;
	std::string cpl_node_name () const = 0;
	virtual int edit_rate_factor () const = 0;
};

/** A 2D (monoscopic) picture asset */
class MonoPictureAsset : public PictureAsset
{
public:
	/** Construct a MonoPictureAsset, generating the MXF from the JPEG2000 files.
	 *  This may take some time; progress is indicated by emission of the Progress signal.
	 *
	 *  @param files Pathnames of JPEG2000 files, in frame order.
	 *  @param directory Directory in which to create MXF file.
	 *  @param mxf_name Name of MXF file to create.
	 *  @param progress Signal to inform of progress.
	 *  @param fps Video frames per second.
	 *  @param intrinsic_duration Total number of frames in the asset.
	 *  @param size Size of images in pixels.
	 *  @param encrypted true if asset should be encrypted.
	 */
	MonoPictureAsset (
		std::vector<std::string> const & files,
		std::string directory,
		std::string mxf_name,
		boost::signals2::signal<void (float)>* progress,
		int fps,
		int intrinsic_duration,
		bool encrypted,
		Size size,
		MXFMetadata const & metadata = MXFMetadata ()
		);

	/** Construct a MonoPictureAsset, generating the MXF from the JPEG2000 files.
	 *  This may take some time; progress is indicated by emission of the Progress signal.
	 *
	 *  @param get_path Functor which returns a JPEG2000 file path for a given frame (frames counted from 0).
	 *  @param directory Directory in which to create MXF file.
	 *  @param mxf_name Name of MXF file to create.
	 *  @param progress Signal to inform of progress.
	 *  @param fps Video frames per second.
	 *  @param intrinsic_duration Total number of frames in the asset.
	 *  @param size Size of images in pixels.
	 *  @param encrypted true if asset should be encrypted.
	 */
	MonoPictureAsset (
		boost::function<std::string (int)> get_path,
		std::string directory,
		std::string mxf_name,
		boost::signals2::signal<void (float)>* progress,
		int fps,
		int intrinsic_duration,
		bool encrypted,
		Size size,
		MXFMetadata const & metadata = MXFMetadata ()
		);

	/** Construct a MonoPictureAsset, reading the MXF from disk.
	 *  @param directory Directory that the MXF is in.
	 *  @param mxf_name The filename of the MXF within `directory'.
	 */
	MonoPictureAsset (std::string directory, std::string mxf_name);

	/** Construct a MonoPictureAsset for progressive writing using
	 *  start_write() and a MonoPictureAssetWriter.
	 *
	 *  @param directory Directory to put the MXF in.
	 *  @param mxf_name Filename of the MXF within this directory.
	 *  @param fps Video frames per second.
	 *  @param size Size in pixels that the picture frames will be.
	 */
	MonoPictureAsset (std::string directory, std::string mxf_name, int fps, Size size);

	/** Start a progressive write to a MonoPictureAsset */
	boost::shared_ptr<PictureAssetWriter> start_write (bool, MXFMetadata const & metadata = MXFMetadata ());

	boost::shared_ptr<const MonoPictureFrame> get_frame (int n) const;
	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;

private:
	std::string path_from_list (int f, std::vector<std::string> const & files) const;
	void construct (boost::function<std::string (int)>, MXFMetadata const &);
	std::string cpl_node_name () const;
	int edit_rate_factor () const;
};

/** A 3D (stereoscopic) picture asset */	
class StereoPictureAsset : public PictureAsset
{
public:
	StereoPictureAsset (std::string directory, std::string mxf_name, int fps, int intrinsic_duration);

	/** Construct a StereoPictureAsset for progressive writing using
	 *  start_write() and a StereoPictureAssetWriter.
	 *
	 *  @param directory Directory to put the MXF in.
	 *  @param mxf_name Filename of the MXF within this directory.
	 *  @param fps Video frames per second.
	 *  @param size Size in pixels that the picture frames will be.
	 */
	StereoPictureAsset (std::string directory, std::string mxf_name, int fps, Size size);

	/** Start a progressive write to a StereoPictureAsset */
	boost::shared_ptr<PictureAssetWriter> start_write (bool, MXFMetadata const & metadata = MXFMetadata ());

	boost::shared_ptr<const StereoPictureFrame> get_frame (int n) const;
	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;

private:
	std::string cpl_node_name () const;
	int edit_rate_factor () const;
};
	

}

#endif
