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

#ifndef LIBDCP_REF_H
#define LIBDCP_REF_H

#include "exceptions.h"
#include "object.h"
#include <boost/shared_ptr.hpp>
#include <string>

namespace dcp {

template<class T>
class Ref
{
public:
	Ref (std::string id)
		: _id (id)
	{}

	Ref (boost::shared_ptr<T> object)
		: _id (object->id ())
		, _object (object)
	{}

	void set_id (std::string id)
	{
		_id = id;
	}

	void resolve (std::list<boost::shared_ptr<Object> > objects)
	{
		typename std::list<boost::shared_ptr<Object> >::iterator i = objects.begin();
		while (i != objects.end() && (*i)->id() != _id) {
			++i;
		}

		if (i != objects.end ()) {
			_object = boost::dynamic_pointer_cast<T> (*i);
		}
	}

	std::string id () const {
		return _id;
	}

	boost::shared_ptr<T> object () const {
		if (!_object) {
			throw UnresolvedRefError (_id);
		}

		return _object;
	}

	T * operator->() const {
		if (!_object) {
			throw UnresolvedRefError (_id);
		}

		return _object.get ();
	}

	bool resolved () const {
		return _object;
	}

private:
	std::string _id;
	boost::shared_ptr<T> _object;
};

}

#endif
