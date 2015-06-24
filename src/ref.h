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
#include "object.h"
#include "util.h"
#include <boost/shared_ptr.hpp>
#include <string>

namespace dcp {

/** @class Ref
 *  @brief A reference to an object which is identified by a universally-unique identifier (UUID).
 *
 *  This class is a `pointer' to a thing.  It will always know the
 *  UUID of the thing, and it may have a shared_ptr to the C++ object
 *  which represents the thing.
 *
 *  If the Ref does not have a shared_ptr it may be given one by
 *  calling resolve() with a list of objects.  The shared_ptr will be
 *  set up using any object on the list which has a matching ID.
 */
template<class T>
class Ref
{
public:
	/** Initialise a Ref with an ID but no shared_ptr */
	Ref (std::string id)
		: _id (id)
	{}

	/** Initialise a Ref with a shared_ptr to an object */
	Ref (boost::shared_ptr<T> object)
		: _id (object->id ())
		, _object (object)
	{}

	/** Set the ID of this Ref */
	void set_id (std::string id)
	{
		_id = id;
	}

	/** Look through a list of objects and copy a shared_ptr to any object
	 *  which matches the ID of this one.
	 */
	void resolve (std::list<boost::shared_ptr<Object> > objects)
	{
		typename std::list<boost::shared_ptr<Object> >::iterator i = objects.begin();
		while (i != objects.end() && !ids_equal ((*i)->id(), _id)) {
			++i;
		}

		if (i != objects.end ()) {
			_object = boost::dynamic_pointer_cast<T> (*i);
		}
	}

	/** @return the ID of the thing that we are pointing to */
	std::string id () const {
		return _id;
	}

	/** @return a shared_ptr to the thing; an UnresolvedRefError is thrown
	 *  if the shared_ptr is not known.
	 */
	boost::shared_ptr<T> object () const {
		if (!_object) {
			throw UnresolvedRefError (_id);
		}

		return _object;
	}

	/** operator-> to access the shared_ptr; an UnresolvedRefError is thrown
	 *  if the shared_ptr is not known.
	 */
	T * operator->() const {
		if (!_object) {
			throw UnresolvedRefError (_id);
		}

		return _object.get ();
	}

	/** @return true if a shared_ptr is known for this Ref */
	bool resolved () const {
		return _object;
	}

private:
	std::string _id;              ///< ID; will always be known
	boost::shared_ptr<T> _object; ///< shared_ptr to the thing, may be null.
};

}

#endif
