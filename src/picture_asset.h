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
 *  @brief An asset made up of JPEG2000 files
 */

#include <openjpeg.h>
#include "mxf_asset.h"
#include "util.h"

namespace libdcp
{

class MonoPictureFrame;	
class StereoPictureFrame;	

/** @brief An asset made up of JPEG2000 files */
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
	 *  @param fps Video Frames per second.
	 *  @param intrinsic_duration Duration of all the frames in the asset.
	 *  @param size Size of video frame images in pixels.
	 */
	PictureAsset (std::string directory, std::string mxf_name, boost::signals2::signal<void (float)>* progress, int fps, int intrinsic_duration, Size size);
	
	/** Write details of this asset to a CPL stream.
	 *  @param s Stream.
	 */
	void write_to_cpl (std::ostream& s) const;

	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (std::string)> note) const;

	Size size () const {
		return _size;
	}

protected:	

	bool frame_buffer_equals (
		int frame, EqualityOptions opt, boost::function<void (std::string)> note,
		uint8_t const * data_A, unsigned int size_A, uint8_t const * data_B, unsigned int size_B
		) const;

	/** picture size in pixels */
	Size _size;
};

class MonoPictureAsset;

struct FrameInfo
{
	FrameInfo (uint64_t o, uint64_t s, std::string h)
		: offset (o)
		, size (s)
		, hash (h)
	{}

	FrameInfo (std::istream& s);

	void write (std::ostream& s);
	
	uint64_t offset;
	uint64_t size;
	std::string hash;
};

/** A helper class for writing to MonoPictureAssets progressively (i.e. writing frame-by-frame,
 *  rather than giving libdcp all the frames in one go).
 *
 *  Objects of this class can only be created with MonoPictureAsset::start_write().
 *
 *  Frames can be written to the MonoPictureAsset by calling write() with a JPEG2000 image
 *  (a verbatim .j2 file).  finalize() must be called after the last frame has been written.
 *  The action of finalize() can't be done in MonoPictureAssetWriter's destructor as it may
 *  throw an exception.
 */
class MonoPictureAssetWriter
{
public:
	~MonoPictureAssetWriter ();

	FrameInfo write (uint8_t* data, int size);
	void fake_write (int size);
	void finalize ();

private:
	friend class MonoPictureAsset;

	MonoPictureAssetWriter (MonoPictureAsset *, bool);
	void start (uint8_t *, int);

	/* no copy construction */
	MonoPictureAssetWriter (MonoPictureAssetWriter const &);
	MonoPictureAssetWriter& operator= (MonoPictureAssetWriter const &);

	/* do this with an opaque pointer so we don't have to include
	   ASDCP headers
	*/
	   
	struct ASDCPState;
	boost::shared_ptr<ASDCPState> _state;

	MonoPictureAsset* _asset;
	/** Number of picture frames written to the asset so far */
	int _frames_written;
	bool _started;
	/** true if finalize() has been called */
	bool _finalized;
	bool _overwrite;
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
	 *  @param intrinsic_duration Length of the whole asset in frames.
	 *  @param size Size of images in pixels.
	 */
	MonoPictureAsset (
		std::vector<std::string> const & files,
		std::string directory,
		std::string mxf_name,
		boost::signals2::signal<void (float)>* progress,
		int fps,
		int intrinsic_duration,
		Size size
		);

	/** Construct a MonoPictureAsset, generating the MXF from the JPEG2000 files.
	 *  This may take some time; progress is indicated by emission of the Progress signal.
	 *
	 *  @param get_path Functor which returns a JPEG2000 file path for a given frame (frames counted from 0).
	 *  @param directory Directory in which to create MXF file.
	 *  @param mxf_name Name of MXF file to create.
	 *  @param progress Signal to inform of progress.
	 *  @param fps Video frames per second.
	 *  @param intrinsic_duration Length of the whole asset in frames.
	 *  @param size Size of images in pixels.
	 */
	MonoPictureAsset (
		boost::function<std::string (int)> get_path,
		std::string directory,
		std::string mxf_name,
		boost::signals2::signal<void (float)>* progress,
		int fps,
		int intrinsic_duration,
		Size size
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
	boost::shared_ptr<MonoPictureAssetWriter> start_write (bool);

	boost::shared_ptr<const MonoPictureFrame> get_frame (int n) const;
	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (std::string)> note) const;

private:
	std::string path_from_list (int f, std::vector<std::string> const & files) const;
	void construct (boost::function<std::string (int)>);
};

/** A 3D (stereoscopic) picture asset */	
class StereoPictureAsset : public PictureAsset
{
public:
	StereoPictureAsset (std::string directory, std::string mxf_name, int fps, int intrinsic_duration);
	
	boost::shared_ptr<const StereoPictureFrame> get_frame (int n) const;
	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (std::string)> note) const;
};
	

}

#endif
