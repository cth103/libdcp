/*
    Copyright (C) 2018 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_VERIFY_H
#define LIBDCP_VERIFY_H

#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <string>
#include <list>
#include <vector>

namespace dcp {

class VerificationNote
{
public:
	/* I've been unable to make mingw happy with ERROR as a symbol, so
	   I'm using a VERIFY_ prefix here.
	*/
	enum Type {
		VERIFY_ERROR,
		VERIFY_WARNING,
		VERIFY_NOTE
	};

	VerificationNote (Type type, std::string note)
		: _type (type)
		, _note (note)
	{}

	Type type () const {
		return _type;
	}

	std::string note () const {
		return _note;
	}

private:
	Type _type;
	std::string _note;
};

std::list<VerificationNote> verify (
	std::vector<boost::filesystem::path> directories,
	boost::function<void (std::string, boost::optional<boost::filesystem::path>)> stage,
	boost::function<void (float)> progress
	);

}

#endif
