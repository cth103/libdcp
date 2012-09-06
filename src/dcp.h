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
class SubtitleAsset;
class Reel;

/** @class DCP dcp.h libdcp/dcp.h
 *  @brief A class to create or read a DCP.
 */
	
class DCP
{
public:
	/** Construct a DCP.
	 *
	 *  This is for making a new DCP that you are going to add assets to.
	 *
	 *  @param directory Directory to write files to.
	 *  @param name Name.
	 *  @param content_kind Content kind.
	 *  @param fps Frames per second.
	 *  @param length Length in frames.
	 */
	DCP (std::string directory, std::string name, ContentKind content_kind, int fps, int length);

	/** Construct a DCP object for an existing DCP.
	 *
	 *  The DCP's XML metadata will be examined, and you can then look at the contents
	 *  of the DCP.
	 *
	 *  @param directory Existing DCP's directory.
	 *  @param read_mxfs true to read MXF files; setting to false can be useful for testing, but
	 *  normally it should be set to true.
	 */
	DCP (std::string directory, bool read_mxfs = true);

	void add_reel (boost::shared_ptr<const Reel> reel);

	/** Write the required XML files to the directory that was
	 *  passed into the constructor.
	 */
	void write_xml () const;

	/** @return the DCP's name, as will be presented on projector
	 *  media servers and theatre management systems.
	 */
	std::string name () const {
		return _name;
	}

	/** @return the type of the content, used by media servers
	 *  to categorise things (e.g. feature, trailer, etc.)
	 */
	ContentKind content_kind () const {
		return _content_kind;
	}

	/** @return the number of frames per second */
	int frames_per_second () const {
		return _fps;
	}

	/** @return the length in frames */
	int length () const {
		return _length;
	}

	std::list<boost::shared_ptr<const Reel> > reels () const {
		return _reels;
	}

	/** Compare this DCP with another, according to various options.
	 *  @param other DCP to compare this one to.
	 *  @param options Options to define just what "equality" means.
	 *  @return An empty list if the DCPs are equal; otherwise a list of messages
	 *  which explain the ways in which they differ.
	 */
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
		std::list<std::string> subtitles;
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
	/** reels */
	std::list<boost::shared_ptr<const Reel> > _reels;
};

}

#endif
