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

/** @file  src/exceptions.cc
 *  @brief Exceptions thrown by libdcp.
 */

#include "exceptions.h"
#include "compose.hpp"

using std::string;
using std::runtime_error;
using namespace dcp;

FileError::FileError (string message, boost::filesystem::path filename, int number)
	: runtime_error (String::compose ("%1 (%2) (error %3)", message, filename.string(), number))
	, _filename (filename)
	, _number (number)
{

}

UnresolvedRefError::UnresolvedRefError (string id)
	: runtime_error (String::compose ("Unresolved reference to asset id %1", id))
{

}

TimeFormatError::TimeFormatError (string bad_time)
	: runtime_error (String::compose ("Bad time string %1", bad_time))
{

}

MissingAssetError::MissingAssetError (boost::filesystem::path path, AssetType type)
	: DCPReadError (
		type == MAIN_PICTURE    ? String::compose ("Missing asset %1 for main picture", path.string()) :
		(type == MAIN_SOUND     ? String::compose ("Missing asset %1 for main sound", path.string()) :
		 (type == MAIN_SUBTITLE ? String::compose ("Missing asset %1 for main subtitle", path.string()) :
		  String::compose ("Missing asset %1", path.string()))))
{

}

NotEncryptedError::NotEncryptedError (string const & what)
	: runtime_error (String::compose ("%1 is not encrypted", what))
{

}


ProgrammingError::ProgrammingError (string file, int line)
	: runtime_error (String::compose ("Programming error at %1:%2", file, line))
{

}

MismatchedStandardError::MismatchedStandardError ()
	: DCPReadError ("DCP contains both Interop and SMPTE parts")
{

}
