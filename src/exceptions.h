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

/** @class FileError
 *  @brief An exception related to a file
 */
class FileError : public std::exception
{
public:
	FileError (std::string message, boost::filesystem::path filename, int number);
	~FileError () throw () {}

	/** @return error message */
	char const * what () const throw () {
		return _message.c_str ();
	}
	
	/** @return filename of file that was involved */
	boost::filesystem::path filename () const {
		return _filename;
	}

	/** @return error number of the error */
	int number () const {
		return _number;
	}

private:
	/** message part */
	std::string _message;
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
class MiscError : public std::exception
{
public:
	MiscError (std::string message) : _message (message) {}
	~MiscError () throw () {}

	/** @return error message */
	char const * what () const throw () {
		return _message.c_str ();
	}

private:
	/** error message */
	std::string _message;
};

/** @class DCPReadError
 *  @brief A DCP read exception
 */
class DCPReadError : public std::exception
{
public:
	DCPReadError (std::string message) : _message (message) {}
	~DCPReadError () throw () {}

	/** @return error message */
	char const * what () const throw () {
		return _message.c_str ();
	}

protected:
	DCPReadError () {}
	
	/** error message */
	std::string _message;
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
class XMLError : public std::exception
{
public:
	XMLError (std::string message) : _message (message) {}
	~XMLError () throw () {}

	/** @return error message */
	char const * what () const throw () {
		return _message.c_str ();
	}

private:
	/** error message */
	std::string _message;
};

/** @class UnresolvedRefError
 *  @brief An exception caused by a reference (by UUID) to something which is not known
 */
class UnresolvedRefError : public std::exception
{
public:
	UnresolvedRefError (std::string id);
	~UnresolvedRefError () throw () {}

	/** @return error message */
	char const * what () const throw () {
		return _message.c_str ();
	}

private:
	std::string _message;
};

/** @class TimeFormatError
 *  @brief A an error with a string passed to LocalTime.
 */
class TimeFormatError : public std::exception
{
public:
	TimeFormatError (std::string bad_time);
	~TimeFormatError () throw () {}

	/** @return error message */
	char const * what () const throw () {
		return _message.c_str ();
	}

private:
	std::string _message;
};

}

#endif
