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

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/

/** @file  src/asset.h
 *  @brief Asset class.
 */

#ifndef LIBDCP_ASSET_H
#define LIBDCP_ASSET_H

#include "object.h"
#include "types.h"
#include "pkl.h"
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
	explicit Asset (boost::filesystem::path file);
	Asset (std::string id, boost::filesystem::path file);

	virtual bool equals (
		boost::shared_ptr<const Asset> other,
		EqualityOptions opt,
		NoteHandler note
		) const;

	/** Write details of the asset to a ASSETMAP.
	 *  @param node Parent node.
	 */
	virtual void write_to_assetmap (xmlpp::Node* node, boost::filesystem::path root) const;

	virtual void add_to_pkl (boost::shared_ptr<PKL> pkl, boost::filesystem::path root) const;

	/** @return the most recent disk file used to read or write this asset, if there is one */
	boost::optional<boost::filesystem::path> file () const {
		return _file;
	}

	void set_file (boost::filesystem::path file) const;

	/** @return the hash of this asset's file */
	std::string hash (boost::function<void (float)> progress = 0) const;

	void set_hash (std::string hash);

protected:

	/** The most recent disk file used to read or write this asset */
	mutable boost::optional<boost::filesystem::path> _file;

	static void write_file_to_assetmap (xmlpp::Node* node, boost::filesystem::path root, boost::filesystem::path file, std::string id);

private:
	friend struct ::asset_test;

	/** @return type string for PKLs for this asset */
	virtual std::string pkl_type (Standard standard) const = 0;

	/** Hash of _file if it has been computed */
	mutable boost::optional<std::string> _hash;
};

}

#endif
