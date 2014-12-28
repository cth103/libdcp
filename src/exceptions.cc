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

/** @file  src/exceptions.cc
 *  @brief Exceptions thrown by libdcp.
 */

#include "exceptions.h"
#include "compose.hpp"

using std::string;
using namespace dcp;

FileError::FileError (string message, boost::filesystem::path filename, int number)
	: StringError (String::compose ("%1 (%2) (error %3)", message, filename.string(), number))
	, _filename (filename)
	, _number (number)
{

}

UnresolvedRefError::UnresolvedRefError (string id)
	: StringError (String::compose ("Unresolved reference to asset id %1", id))
{

}

TimeFormatError::TimeFormatError (string bad_time)
	: StringError (String::compose ("Bad time string %1", bad_time))
{

}

MissingAssetError::MissingAssetError (boost::filesystem::path path, AssetType type)
	: _path (path)
	, _type (type)
{
	string type_name;
	switch (_type) {
	case MAIN_PICTURE:
		type_name = " for main picture";
		break;
	case MAIN_SOUND:
		type_name = " for main sound";
		break;
	case MAIN_SUBTITLE:
		type_name = " for main subtitle";
		break;
	case UNKNOWN:
		break;
	}
	
	_message = String::compose ("Missing asset %1%2", path.string(), type_name);
}

NotEncryptedError::NotEncryptedError (string const & what)
	: StringError (String::compose ("%1 is not encrypted", what))
{

}


ProgrammingError::ProgrammingError (string file, int line)
	: StringError (String::compose ("Programming error at %1:%2", file, line))
{

}
