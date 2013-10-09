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

/** @brief An exception related to a file */
class FileError : public std::exception
{
public:
	FileError (std::string const & message, boost::filesystem::path filename)
		: _message (message + "(" + filename.string() + ")")
		, _filename (filename)
	{}
			    
	~FileError () throw () {}

	/** @return error message */
	char const * what () const throw () {
		return _message.c_str ();
	}
	
	/** @return filename of file that was involved */
	boost::filesystem::path filename () const {
		return _filename;
	}

private:
	/** message part */
	std::string _message;
	/** filename of file that was involved */
	boost::filesystem::path _filename;
};

/** @brief An exception related to an MXF file */
class MXFFileError : public FileError
{
public:
	MXFFileError (std::string const & message, boost::filesystem::path filename)
		: FileError (message, filename)
	{}
};
	
/** @brief A miscellaneous exception */
class MiscError : public std::exception
{
public:
	MiscError (std::string const & message) : _message (message) {}
	~MiscError () throw () {}

	/** @return error message */
	char const * what () const throw () {
		return _message.c_str ();
	}

private:
	/** error message */
	std::string _message;
};

/** @brief A DCP read exception */
class DCPReadError : public std::exception
{
public:
	DCPReadError (std::string const & message) : _message (message) {}
	~DCPReadError () throw () {}

	/** @return error message */
	char const * what () const throw () {
		return _message.c_str ();
	}

private:
	/** error message */
	std::string _message;
};

/** @brief An XML error */
class XMLError : public std::exception
{
public:
	XMLError (std::string const & message) : _message (message) {}
	~XMLError () throw () {}

	/** @return error message */
	char const * what () const throw () {
		return _message.c_str ();
	}

private:
	/** error message */
	std::string _message;
};
	
}

#endif
