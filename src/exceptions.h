/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_EXCEPTIONS_H
#define LIBDCP_EXCEPTIONS_H

#include <boost/filesystem.hpp>

/** @file  src/exceptions.h
 *  @brief Exceptions thrown by libdcp.
 */

namespace dcp
{

class StringError : public std::exception
{
public:
	StringError () {}
	StringError (std::string message)
		: _message (message)
	{}
			    
	~StringError () throw () {}

	/** @return error message */
	char const * what () const throw () {
		return _message.c_str ();
	}

protected:
	std::string _message;
};

/** @class FileError
 *  @brief An exception related to a file
 */
class FileError : public StringError
{
public:
	FileError (std::string message, boost::filesystem::path filename, int number);
	~FileError () throw () {}

	/** @return filename of file that was involved */
	boost::filesystem::path filename () const {
		return _filename;
	}

	/** @return error number of the error */
	int number () const {
		return _number;
	}

private:
	/** filename of file that was involved */
	boost::filesystem::path _filename;
	int _number;
};

/** @class MXFFileError
 *  @brief An exception related to an MXF file
 */
class MXFFileError : public FileError
{
public:
	MXFFileError (std::string message, boost::filesystem::path filename, int number)
		: FileError (message, filename, number)
	{}
};
	
/** @class MiscError
 *  @brief A miscellaneous exception
 */
class MiscError : public StringError
{
public:
	MiscError (std::string message)
		: StringError (message)
	{}
};

/** @class DCPReadError
 *  @brief A DCP read exception
 */
class DCPReadError : public StringError
{
public:
	DCPReadError (std::string message)
		: StringError (message)
	{}

protected:
	DCPReadError () {}
};

/** @class MissingAssetError
 *  @brief An error of a missing asset.
 */
class MissingAssetError : public DCPReadError
{
public:
	enum AssetType {
		MAIN_PICTURE,  //< main picture is missing
		MAIN_SOUND,    //< main sound is missing
		MAIN_SUBTITLE, //< main subtitle is missing
		UNKNOWN        //< something is missing but we don't know what
	};
	
	MissingAssetError (boost::filesystem::path, AssetType = UNKNOWN);
	~MissingAssetError () throw () {}

private:
	boost::filesystem::path _path;
	AssetType _type;
};

/** @class XMLError
 *  @brief An XML error
 */
class XMLError : public StringError
{
public:
	XMLError (std::string message)
		: StringError (message)
	{}
};

/** @class UnresolvedRefError
 *  @brief An exception caused by a reference (by UUID) to something which is not known
 */
class UnresolvedRefError : public StringError
{
public:
	UnresolvedRefError (std::string id);
};

/** @class TimeFormatError
 *  @brief A an error with a string passed to LocalTime.
 */
class TimeFormatError : public StringError
{
public:
	TimeFormatError (std::string bad_time);
};

class NotEncryptedError : public StringError
{
public:
	NotEncryptedError (std::string const & what);
	~NotEncryptedError () throw () {}
};
	
class ProgrammingError : public StringError
{
public:
	ProgrammingError (std::string file, int line);
};

}

#endif
