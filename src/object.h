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


/** @file  src/object.h
 *  @brief Object class
 */


#ifndef LIBDCP_OBJECT_H
#define LIBDCP_OBJECT_H


#include <string>


struct write_interop_subtitle_test;
struct write_interop_subtitle_test2;
struct write_interop_subtitle_test3;
struct write_smpte_subtitle_test;
struct write_smpte_subtitle_test2;
struct write_smpte_subtitle_test3;
struct sync_test2;


namespace dcp {


/** @class Object
 *  @brief Some part of a DCP that has a UUID
 */
class Object
{
public:
	/** Create an Object with a random ID */
	Object ();

	/** Create an Object with a given ID.
	 *  @param id ID to use.
	 */
	explicit Object (std::string id);

	Object (Object const&) = delete;
	Object& operator= (Object const&) = delete;

	virtual ~Object () {}

	std::string id () const {
		return _id;
	}

protected:
	friend struct ::write_interop_subtitle_test;
	friend struct ::write_interop_subtitle_test2;
	friend struct ::write_interop_subtitle_test3;
	friend struct ::write_smpte_subtitle_test;
	friend struct ::write_smpte_subtitle_test2;
	friend struct ::write_smpte_subtitle_test3;
	friend struct ::sync_test2;

	std::string _id;
};

}

#endif
