/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

namespace libdcp
{

class StringError : public std::exception
{
public:
	StringError (std::string const & message)
		: _message (message)
	{}

	~StringError () throw () {}

	/** @return error message */
	char const * what () const throw () {
		return _message.c_str ();
	}

private:
	std::string _message;
};

/** @brief An exception related to a file */
class FileError : public StringError
{
public:
	FileError (std::string const & message, boost::filesystem::path filename, int number);
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

/** @brief An exception related to an MXF file */
class MXFFileError : public FileError
{
public:
	MXFFileError (std::string const & message, boost::filesystem::path filename, int number)
		: FileError (message, filename, number)
	{}
};
	
/** @brief A miscellaneous exception */
class MiscError : public StringError
{
public:
	MiscError (std::string const & message)
		: StringError (message)
	{}
	
	~MiscError () throw () {}
};

/** @brief A DCP read exception */
class DCPReadError : public StringError
{
public:
	DCPReadError (std::string const & message)
		: StringError (message)
	{}
	
	~DCPReadError () throw () {}
};

/** @brief An XML error */
class XMLError : public StringError
{
public:
	XMLError (std::string const & message)
		: StringError (message)
	{}

	~XMLError () throw () {}
};

class NotEncryptedError : public StringError
{
public:
	NotEncryptedError (std::string const & asset);
	~NotEncryptedError () throw () {}
};
	
}

#endif
