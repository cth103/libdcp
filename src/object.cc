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

/** @file  src/object.cc
 *  @brief Object class.
 */

#include "object.h"
#include "dcp_assert.h"
#include "util.h"

using std::string;
using namespace dcp;

/** Create an Object with a random ID. */
Object::Object ()
	: _id (make_uuid ())
{

}

/** Create an Object with a given ID.
 *  @param id ID to use.
 */
Object::Object (string id)
	: _id (id)
{
	DCP_ASSERT (_id.substr(0, 9) != "urn:uuid:");
}
