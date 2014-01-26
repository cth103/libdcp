/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

/** @file  src/asset.h
 *  @brief Asset class.
 */ 

#ifndef LIBDCP_ASSET_H
#define LIBDCP_ASSET_H

#include "object.h"
#include "types.h"
#include <boost/filesystem.hpp>
#include <boost/function.hpp>

namespace xmlpp {
	class Node;
}

namespace dcp {

/** @class Asset
 *  @brief Parent class for DCP assets, i.e. picture/sound/subtitles and CPLs.
 *
 *  Note that this class is not used for ReelAssets; they are just for the metadata
 *  that gets put into &lt;Reel&gt;s.
 */
class Asset : public Object
{
public:
	Asset ();
	Asset (boost::filesystem::path file);
	Asset (std::string id);

	virtual std::string pkl_type () const = 0;
	virtual bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;
	
	/** Write details of the asset to a PKL AssetList node.
	 *  @param node Parent node.
	 */
	void write_to_pkl (xmlpp::Node* node) const;
	void write_to_assetmap (xmlpp::Node* node) const;

	boost::filesystem::path file () const {
		return _file;
	}

	void set_file (boost::filesystem::path file) {
		_file = file;
	}

	std::string hash () const;

protected:
	friend class MXFWriter;
	
	boost::filesystem::path _file;
	mutable std::string _hash;
};

}

#endif
