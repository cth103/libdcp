/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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
#include <boost/bind.hpp>
#include <boost/optional.hpp>

namespace xmlpp {
	class Node;
}

struct asset_test;

namespace dcp {

/** @class Asset
 *  @brief Parent class for DCP assets, i.e. picture, sound, subtitles, CPLs, fonts.
 *
 *  Note that this class is not used for ReelAssets; those are just for the metadata
 *  that gets put into &lt;Reel&gt;s.
 */
class Asset : public Object
{
public:
	Asset ();
	Asset (boost::filesystem::path file);
	Asset (std::string id, boost::filesystem::path file);

	virtual bool equals (
		boost::shared_ptr<const Asset> other,
		EqualityOptions opt,
		NoteHandler note
		) const;

	/** Write details of the asset to a ASSETMAP.
	 *  @param node Parent node.
	 */
	void write_to_assetmap (xmlpp::Node* node, boost::filesystem::path root) const;

	/** Write details of the asset to a PKL AssetList node.
	 *  @param node Parent node.
	 *  @param standard Standard to use.
	 */
	void write_to_pkl (xmlpp::Node* node, boost::filesystem::path root, Standard standard) const;

	/** @return the most recent disk file used to read or write this asset; may be empty */
	boost::filesystem::path file () const {
		return _file;
	}

	void set_file (boost::filesystem::path file) const;

	/** @return the hash of this asset's file */
	std::string hash (boost::function<void (float)> progress = 0) const;

protected:

	/** The most recent disk file used to read or write this asset; may be empty */
	mutable boost::filesystem::path _file;

private:
	friend struct ::asset_test;

	virtual std::string pkl_type (Standard standard) const = 0;

	/** Hash of _file if it has been computed */
	mutable boost::optional<std::string> _hash;
};

}

#endif
