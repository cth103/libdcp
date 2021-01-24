/*
    Copyright (C) 2016-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/name_format.h
 *  @brief NameFormat class
 */


#ifndef LIBDCP_NAME_FORMAT
#define LIBDCP_NAME_FORMAT


#include <string>
#include <boost/optional.hpp>
#include <map>
#include <list>


namespace dcp {


class NameFormat
{
public:
	NameFormat () {}

	NameFormat (std::string specification)
		: _specification (specification)
	{}

	std::string specification () const {
		return _specification;
	}

	void set_specification (std::string specification) {
		_specification = specification;
	}

	typedef std::map<char, std::string> Map;

	/** @param values Values to replace our specifications with; e.g.
	 *  if the specification contains %c it will be be replaced with the
	 *  value corresponding to the key 'c'.
	 *  @param suffix Suffix to add on after processing the specification.
	 *  @param ignore Any specification characters in this string will not
	 *  be replaced, but left as-is.
	 */
	std::string get (Map, std::string suffix, std::string ignore = "") const;

private:
	std::string _specification;
};


extern bool operator== (NameFormat const & a, NameFormat const & b);


}


#endif
