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
#include "asset.h"

namespace libdcp
{

/** @brief An asset made up of JPEG2000 files */
class PictureAsset : public Asset
{
public:
	/** Construct a PictureAsset, generating the MXF from the JPEG2000 files.
	 *  This may take some time; progress is indicated by emission of the Progress signal.
	 *  @param files Pathnames of JPEG2000 files, in frame order.
	 *  @param directory Directory in which to create MXF file.
	 *  @param mxf_name Name of MXF file to create.
	 *  @param progress Signal to inform of progress.
	 *  @param fps Frames per second.
	 *  @param length Length in frames.
	 *  @param width Width of images in pixels.
	 *  @param height Height of images in pixels.
	 */
	PictureAsset (
		std::vector<std::string> const & files,
		std::string directory,
		std::string mxf_name,
		sigc::signal1<void, float>* progress,
		int fps,
		int length,
		int width,
		int height
		);

	/** Construct a PictureAsset, generating the MXF from the JPEG2000 files.
	 *  This may take some time; progress is indicated by emission of the Progress signal.
	 *  @param get_path Functor which returns a JPEG2000 file path for a given frame (frames counted from 0).
	 *  @param directory Directory in which to create MXF file.
	 *  @param mxf_name Name of MXF file to create.
	 *  @param progress Signal to inform of progress.
	 *  @param fps Frames per second.
	 *  @param length Length in frames.
	 *  @param width Width of images in pixels.
	 *  @param height Height of images in pixels.
	 */
	PictureAsset (
		sigc::slot<std::string, int> get_path,
		std::string directory,
		std::string mxf_name,
		sigc::signal1<void, float>* progress,
		int fps,
		int length,
		int width,
		int height
		);

	PictureAsset (std::string directory, std::string mxf_name, int fps, int length, int width, int height);
	
	/** Write details of this asset to a CPL stream.
	 *  @param s Stream.
	 */
	void write_to_cpl (std::ostream& s) const;

	std::list<std::string> equals (boost::shared_ptr<const Asset> other, EqualityOptions opt) const;
	
private:
	std::string path_from_list (int f, std::vector<std::string> const & files) const;
	void construct (sigc::slot<std::string, int>);
	opj_image_t* decompress_j2k (uint8_t* data, int64_t size) const;
	
	/** picture width in pixels */
	int _width;
	/** picture height in pixels */
	int _height;
};

}
