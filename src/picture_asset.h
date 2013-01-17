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
	PictureAsset (std::string directory, std::string mxf_name, boost::signals2::signal<void (float)>* progress, int fps, int intrinsic_duration);
	
	/** Write details of this asset to a CPL stream.
	 *  @param s Stream.
	 */
	void write_to_cpl (std::ostream& s) const;

	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, std::list<std::string>& notes) const;

	Size size () const {
		return _size;
	}

protected:	

	bool frame_buffer_equals (
		int frame, EqualityOptions opt, std::list<std::string>& notes,
		uint8_t const * data_A, unsigned int size_A, uint8_t const * data_B, unsigned int size_B
		) const;

	/** picture size in pixels */
	Size _size;
};

/** A 2D (monoscopic) picture asset */
class MonoPictureAsset : public PictureAsset
{
public:
	/** Construct a PictureAsset, generating the MXF from the JPEG2000 files.
	 *  This may take some time; progress is indicated by emission of the Progress signal.
	 *  @param files Pathnames of JPEG2000 files, in frame order.
	 *  @param directory Directory in which to create MXF file.
	 *  @param mxf_name Name of MXF file to create.
	 *  @param progress Signal to inform of progress.
	 *  @param fps Frames per second.
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

	/** Construct a PictureAsset, generating the MXF from the JPEG2000 files.
	 *  This may take some time; progress is indicated by emission of the Progress signal.
	 *  @param get_path Functor which returns a JPEG2000 file path for a given frame (frames counted from 0).
	 *  @param directory Directory in which to create MXF file.
	 *  @param mxf_name Name of MXF file to create.
	 *  @param progress Signal to inform of progress.
	 *  @param fps Frames per second.
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

	MonoPictureAsset (std::string directory, std::string mxf_name, int fps, int intrinsic_duration);
	
	boost::shared_ptr<const MonoPictureFrame> get_frame (int n) const;
	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, std::list<std::string>& notes) const;

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
	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, std::list<std::string>& notes) const;
};
	

}
