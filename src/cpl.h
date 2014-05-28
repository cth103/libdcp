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

#ifndef LIBDCP_CPL_H
#define LIBDCP_CPL_H

#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <libxml++/libxml++.h>
#include "types.h"
#include "certificates.h"
#include "key.h"

namespace libdcp {

namespace parse {
	class AssetMap;
	class AssetMapAsset;
}
	
class Asset;
class Reel;
class XMLMetadata;
class MXFMetadata;
class Signer;
class KDM;
	
/** @brief A CPL within a DCP */
class CPL
{
public:
	CPL (boost::filesystem::path directory, std::string name, ContentKind content_kind, int length, int frames_per_second);
	CPL (boost::filesystem::path, std::string file, std::list<PathAssetMap> asset_maps, bool require_mxfs = true);

	void add_reel (boost::shared_ptr<Reel> reel);
	
	/** @return the length in frames */
	int length () const {
		return _length;
	}

	/** @return the type of the content, used by media servers
	 *  to categorise things (e.g. feature, trailer, etc.)
	 */
	ContentKind content_kind () const {
		return _content_kind;
	}

	std::list<boost::shared_ptr<Reel> > reels () const {
		return _reels;
	}

	/** @return the CPL's name, as will be presented on projector
	 *  media servers and theatre management systems.
	 */
	std::string name () const {
		return _name;
	}

	/** @return the number of frames per second */
	int frames_per_second () const {
		return _fps;
	}

	std::list<boost::shared_ptr<const Asset> > assets () const;

	bool encrypted () const;

	void set_mxf_keys (Key);

	std::string id () const {
		return _id;
	}

	boost::filesystem::path filename () const;
	
	bool equals (CPL const & other, EqualityOptions options, boost::function<void (NoteType, std::string)> note) const;
	
	void write_xml (bool, XMLMetadata const &, boost::shared_ptr<const Signer>) const;
	void write_to_assetmap (xmlpp::Node *) const;
	void write_to_pkl (xmlpp::Node *, bool) const;

	void add_kdm (KDM const &);
	
private:
	std::pair<std::string, boost::shared_ptr<const parse::AssetMapAsset> > asset_from_id (std::list<PathAssetMap>, std::string id) const;
	
	boost::filesystem::path _directory;
	/** the name of the DCP */
	std::string _name;
	/** the content kind of the CPL */
	ContentKind _content_kind;
	/** length in frames */
	mutable int _length;
	/** frames per second */
	int _fps;
	/** reels */
	std::list<boost::shared_ptr<Reel> > _reels;

	/** our UUID */
	std::string _id;
	/** a SHA1 digest of our XML */
	mutable std::string _digest;
};

}

#endif
