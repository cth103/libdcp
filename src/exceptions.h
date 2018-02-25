/*
    Copyright (C) 2012-2018 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_EXCEPTIONS_H
#define LIBDCP_EXCEPTIONS_H

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

/** @file  src/exceptions.h
 *  @brief Exceptions thrown by libdcp.
 */

namespace dcp
{

/** @class FileError
 *  @brief An exception related to a file
 */
class FileError : public std::runtime_error
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
class MiscError : public std::runtime_error
{
public:
	explicit MiscError (std::string message)
		: std::runtime_error (message)
	{}
};

/** @class DCPReadError
 *  @brief A DCP read exception
 */
class DCPReadError : public std::runtime_error
{
public:
	explicit DCPReadError (std::string message)
		: std::runtime_error(message)
	{}

	DCPReadError (std::string message, std::string detail);

	~DCPReadError() throw () {}

	std::string message () const {
		return _message;
	}

	boost::optional<std::string> detail () const {
		return _detail;
	}

private:
	std::string _message;
	boost::optional<std::string> _detail;
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
};

/** @class XMLError
 *  @brief An XML error
 */
class XMLError : public std::runtime_error
{
public:
	explicit XMLError (std::string message)
		: std::runtime_error (message)
	{}
};

/** @class UnresolvedRefError
 *  @brief An exception caused by a reference (by UUID) to something which is not known
 */
class UnresolvedRefError : public std::runtime_error
{
public:
	explicit UnresolvedRefError (std::string id);
};

/** @class TimeFormatError
 *  @brief A an error with a string passed to LocalTime.
 */
class TimeFormatError : public std::runtime_error
{
public:
	explicit TimeFormatError (std::string bad_time);
};

/** @class NotEncryptedError
 *  @brief An error raised when creating a DecryptedKDM object for assets that are not
 *  encrypted.
 */
class NotEncryptedError : public std::runtime_error
{
public:
	explicit NotEncryptedError (std::string const & what);
	~NotEncryptedError () throw () {}
};

/** @class ProgrammingError
 *  @brief An exception thrown when a DCP_ASSERT fails; something that should not happen.
 */
class ProgrammingError : public std::runtime_error
{
public:
	ProgrammingError (std::string file, int line);
};

class MismatchedStandardError : public DCPReadError
{
public:
	MismatchedStandardError ();
};

class KDMDecryptionError : public std::runtime_error
{
public:
	KDMDecryptionError (std::string message, int cipher_length, int modulus_dmax);
};

class KDMFormatError : public std::runtime_error
{
public:
	KDMFormatError (std::string message);
};

class CertificateChainError : public std::runtime_error
{
public:
	CertificateChainError (std::string message);
};

}

#endif
