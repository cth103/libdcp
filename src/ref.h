/*
    Copyright (C) 2014-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/ref.h
 *  @brief Ref class
 */


#ifndef LIBDCP_REF_H
#define LIBDCP_REF_H


#include "exceptions.h"
#include "asset.h"
#include "util.h"
#include <memory>
#include <string>


namespace dcp {


/** @class Ref
 *  @brief A reference to an asset which is identified by a universally-unique identifier (UUID)
 *
 *  This class is a `pointer' to a thing.  It will always know the
 *  UUID of the thing, and it may have a shared_ptr to the C++ object
 *  which represents the thing.
 *
 *  If the Ref does not have a shared_ptr it may be given one by
 *  calling resolve() with a vector of assets.  The shared_ptr will be
 *  set up using any object on the vector which has a matching ID.
 */
class Ref
{
public:
	/** Initialise a Ref with an ID but no shared_ptr */
	explicit Ref (std::string id)
		: _id (id)
	{}

	/** Initialise a Ref with a shared_ptr to an asset */
	explicit Ref (std::shared_ptr<Asset> asset)
		: _id (asset->id ())
		, _asset (asset)
	{}

	/** Set the ID of this Ref */
	void set_id (std::string id)
	{
		_id = id;
	}

	/** Look through a list of assets and copy a shared_ptr to any asset
	 *  which matches the ID of this one
	 */
	void resolve (std::vector<std::shared_ptr<Asset>> assets);

	/** @return the ID of the thing that we are pointing to */
	std::string id () const {
		return _id;
	}

	/** @return a shared_ptr to the thing; an UnresolvedRefError is thrown
	 *  if the shared_ptr is not known
	 */
	std::shared_ptr<Asset> asset () const {
		if (!_asset) {
			throw UnresolvedRefError (_id);
		}

		return _asset;
	}

	/** operator-> to access the shared_ptr; an UnresolvedRefError is thrown
	 *  if the shared_ptr is not known
	 */
	Asset * operator->() const {
		if (!_asset) {
			throw UnresolvedRefError (_id);
		}

		return _asset.get ();
	}

	/** @return true if a shared_ptr is known for this Ref */
	bool resolved () const {
		return static_cast<bool>(_asset);
	}

private:
	std::string _id;             ///< ID; will always be known
	std::shared_ptr<Asset> _asset; ///< shared_ptr to the thing, may be null.
};


}


#endif
