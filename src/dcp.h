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

/** @file  src/dcp.h
 *  @brief A class to create a DCP.
 */

#ifndef LIBDCP_DCP_H
#define LIBDCP_DCP_H

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <sigc++/sigc++.h>
#include "types.h"

namespace xmlpp {
	class Node;
}

/** @brief Namespace for everything in libdcp */
namespace libdcp
{

class Asset;	
class PictureAsset;
class SoundAsset;

/** @class DCP dcp.h libdcp/dcp.h
 *  @brief A class to create or read a DCP.
 */
	
class DCP
{
public:
	/** Construct a DCP.
	 *  @param directory Directory to write files to.
	 *  @param name Name.
	 *  @param content_kind Content kind.
	 *  @param fps Frames per second.
	 *  @param length Length in frames.
	 */
	DCP (std::string directory, std::string name, ContentKind content_kind, int fps, int length);

	DCP (std::string directory);

	/** Add a sound asset.
	 *  @param files Pathnames of WAV files to use in the order Left, Right,
	 *  Centre, Lfe (sub), Left surround, Right surround; not all files need
	 *  to be present.
	 */
	void add_sound_asset (std::vector<std::string> const & files);

	/** Add a sound asset.
	 *  @param get_path Functor to get the path to the WAV for a given channel.
	 *  @param channels Number of channels.
	 */
	void add_sound_asset (sigc::slot<std::string, Channel> get_path, int channels);

	/** Add a picture asset.
	 *  @param files Pathnames of JPEG2000 files, in frame order.
	 *  @param width Width of images in pixels.
	 *  @param height Height of images in pixels.
	 */
	void add_picture_asset (std::vector<std::string> const & files, int width, int height);

	/** Add a picture asset.
	 *  @param get_path Functor to get path to the JPEG2000 for a given frame.
	 *  @param width Width of images in pixels.
	 *  @param height Height of images in pixels.
	 */
	void add_picture_asset (sigc::slot<std::string, int> get_path, int width, int height);

	/** Write the required XML files to the directory that was
	 *  passed into the constructor.
	 */
	void write_xml () const;

	std::string name () const {
		return _name;
	}

	ContentKind content_kind () const {
		return _content_kind;
	}

	int frames_per_second () const {
		return _fps;
	}

	int length () const {
		return _length;
	}

	boost::shared_ptr<const PictureAsset> picture_asset () const;
	boost::shared_ptr<const SoundAsset> sound_asset () const;

	std::list<std::string> equals (DCP const & other, EqualityOptions options) const;

	/** Emitted with a parameter between 0 and 1 to indicate progress
	 *  for long jobs.
	 */
	sigc::signal1<void, float> Progress;

private:

	/** Write the CPL file.
	 *  @param cpl_uuid UUID to use.
	 *  @return CPL pathname.
	 */
	std::string write_cpl (std::string cpl_uuid) const;

	/** Write the PKL file.
	 *  @param pkl_uuid UUID to use.
	 *  @param cpl_uuid UUID of the CPL file.
	 *  @param cpl_digest SHA digest of the CPL file.
	 *  @param cpl_length Length of the CPL file in bytes.
	 */
	std::string write_pkl (std::string pkl_uuid, std::string cpl_uuid, std::string cpl_digest, int cpl_length) const;
	
	/** Write the VOLINDEX file */
	void write_volindex () const;

	/** Write the ASSETMAP file.
	 *  @param cpl_uuid UUID of our CPL.
	 *  @param cpl_length Length of our CPL in bytes.
	 *  @param pkl_uuid UUID of our PKL.
	 *  @param pkl_length Length of our PKL in bytes.
	 */
	void write_assetmap (std::string cpl_uuid, int cpl_length, std::string pkl_uuid, int pkl_length) const;

	struct Files {
		std::string cpl;
		std::string pkl;
		std::string asset_map;
	};

	void scan (Files& files, std::string directory) const;

	/** the directory that we are writing to */
	std::string _directory;
	/** the name of the DCP */
	std::string _name;
	/** the content kind of the DCP */
	ContentKind _content_kind;
	/** frames per second */
	int _fps;
	/** length in frames */
	int _length;
	/** assets */
	std::list<boost::shared_ptr<Asset> > _assets;
};

}

#endif
