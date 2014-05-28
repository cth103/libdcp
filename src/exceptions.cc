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

#include "exceptions.h"
#include "compose.hpp"

using std::string;
using namespace libdcp;

FileError::FileError (string const & message, boost::filesystem::path filename, int number)
	: StringError (String::compose ("%1 (%2) (error %3)", message, filename.string(), number))
	, _filename (filename)
	, _number (number)
{

}

NotEncryptedError::NotEncryptedError (string const & what)
	: StringError (String::compose ("%1 asset is not encrypted", what))
{

}
