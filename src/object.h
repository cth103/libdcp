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

/** @file  src/object.h
 *  @brief Object class.
 */

#ifndef LIBDCP_OBJECT_H
#define LIBDCP_OBJECT_H

#include <boost/noncopyable.hpp>
#include <string>

class write_subtitle_test;

namespace dcp {

/** @class Object
 *  @brief Some part of a DCP that has a UUID.
 */
class Object : public boost::noncopyable
{
public:
	Object ();
	Object (std::string id);
	virtual ~Object () {}

	/** @return ID */
	std::string id () const {
		return _id;
	}

protected:
	friend class ::write_subtitle_test;

	/** ID */
	std::string _id;
};

}

#endif
