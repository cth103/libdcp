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

/** @file  src/ref.h
 *  @brief Ref class.
 */

#ifndef LIBDCP_REF_H
#define LIBDCP_REF_H

#include "exceptions.h"
#include "asset.h"
#include "util.h"
#include <boost/shared_ptr.hpp>
#include <string>

namespace dcp {

/** @class Ref
 *  @brief A reference to an asset which is identified by a universally-unique identifier (UUID).
 *
 *  This class is a `pointer' to a thing.  It will always know the
 *  UUID of the thing, and it may have a shared_ptr to the C++ object
 *  which represents the thing.
 *
 *  If the Ref does not have a shared_ptr it may be given one by
 *  calling resolve() with a list of assets.  The shared_ptr will be
 *  set up using any object on the list which has a matching ID.
 */
class Ref
{
public:
	/** Initialise a Ref with an ID but no shared_ptr */
	Ref (std::string id)
		: _id (id)
	{}

	/** Initialise a Ref with a shared_ptr to an asset */
	Ref (boost::shared_ptr<Asset> asset)
		: _id (asset->id ())
		, _asset (asset)
	{}

	/** Set the ID of this Ref */
	void set_id (std::string id)
	{
		_id = id;
	}

	void resolve (std::list<boost::shared_ptr<Asset> > assets);

	/** @return the ID of the thing that we are pointing to */
	std::string id () const {
		return _id;
	}

	/** @return a shared_ptr to the thing; an UnresolvedRefError is thrown
	 *  if the shared_ptr is not known.
	 */
	boost::shared_ptr<Asset> asset () const {
		if (!_asset) {
			throw UnresolvedRefError (_id);
		}

		return _asset;
	}

	/** operator-> to access the shared_ptr; an UnresolvedRefError is thrown
	 *  if the shared_ptr is not known.
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
	boost::shared_ptr<Asset> _asset; ///< shared_ptr to the thing, may be null.
};

}

#endif
