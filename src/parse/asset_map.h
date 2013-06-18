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

/** @file  src/asset_map.h
 *  @brief Classes used to parse a AssetMap.
 */

#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <libcxml/cxml.h>

namespace libdcp {

namespace parse {

/** @class Chunk
 *  @brief A simple parser for and representation of a \<Chunk\> node within an asset map.
 */
class Chunk
{
public:
	Chunk ();
	Chunk (boost::shared_ptr<const cxml::Node> node);

	std::string path;
	int64_t volume_index;
	int64_t offset;
	int64_t length;
};

/** @class AssetMapAsset
 *  @brief A simple parser for and representation of an \<AssetMap\> node within an asset map.
 */
class AssetMapAsset
{
public:
	AssetMapAsset ();
	AssetMapAsset (boost::shared_ptr<const cxml::Node> node);

	std::string id;
	std::string packing_list;
	std::list<boost::shared_ptr<Chunk> > chunks;
};

/** @class AssetMap
 *  @brief A simple parser for and representation of an asset map file.
 */
class AssetMap
{
public:
	AssetMap (std::string file);

	boost::shared_ptr<AssetMapAsset> asset_from_id (std::string id) const;
	
	std::string id;
	std::string creator;
	int64_t volume_count;
	std::string issue_date;
	std::string issuer;
	std::list<boost::shared_ptr<AssetMapAsset> > assets;
};

}

}
