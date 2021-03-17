/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/exceptions.h
 *  @brief Exceptions thrown by libdcp
 */


#ifndef LIBDCP_EXCEPTIONS_H
#define LIBDCP_EXCEPTIONS_H


#include <boost/filesystem.hpp>
#include <boost/optional.hpp>


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


/** @class ReadError
 *  @brief Any error that occurs when reading data from a DCP
 */
class ReadError : public std::runtime_error
{
public:
	explicit ReadError (std::string message)
		: std::runtime_error(message)
		, _message(message)
	{}

	ReadError (std::string message, std::string detail);

	~ReadError() throw () {}

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


/** @class J2KDecompressionError
 *  @brief An error that occurs during decompression of JPEG2000 data
 */
class J2KDecompressionError : public ReadError
{
public:
	explicit J2KDecompressionError (std::string message)
		: ReadError (message)
	{}
};


class BadContentKindError : public ReadError
{
public:
	BadContentKindError (std::string content_kind);
};


/** @class MissingAssetmapError
 *  @brief Thrown when no ASSETMAP was found when trying to read a DCP
 */
class MissingAssetmapError : public ReadError
{
public:
	explicit MissingAssetmapError (boost::filesystem::path dir);
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
 *  @brief A an error with a string passed to LocalTime
 */
class TimeFormatError : public std::runtime_error
{
public:
	explicit TimeFormatError (std::string bad_time);
};


/** @class NotEncryptedError
 *  @brief An error raised when creating a DecryptedKDM object for assets that are not
 *  encrypted
 */
class NotEncryptedError : public std::runtime_error
{
public:
	explicit NotEncryptedError (std::string const & what);
	~NotEncryptedError () throw () {}
};


/** @class ProgrammingError
 *  @brief An exception thrown when a DCP_ASSERT fails; something that should not happen
 */
class ProgrammingError : public std::runtime_error
{
public:
	ProgrammingError (std::string file, int line);
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


class MissingSubtitleImageError : public std::runtime_error
{
public:
	MissingSubtitleImageError (std::string id);
};


class BadKDMDateError : public std::runtime_error
{
public:
	BadKDMDateError (bool starts_too_early);

	bool starts_too_early () const {
		return _starts_too_early;
	}

private:
	bool _starts_too_early;
};


class StartCompressionError : public std::runtime_error
{
public:
	explicit StartCompressionError (boost::optional<int> code = boost::optional<int>());
	~StartCompressionError () throw () {}

	boost::optional<int> code () const {
		return _code;
	}

private:
	boost::optional<int> _code;
};


class CombineError : public std::runtime_error
{
public:
	explicit CombineError (std::string message);
};


class LanguageTagError : public std::runtime_error
{
public:
	LanguageTagError (std::string message);
};


class BadSettingError : public std::runtime_error
{
public:
	BadSettingError (std::string message);
};


class DuplicateIdError : public std::runtime_error
{
public:
	DuplicateIdError (std::string message);
};


class MainSoundConfigurationError : public std::runtime_error
{
public:
	MainSoundConfigurationError (std::string s);
};


class UnknownChannelIdError : public std::runtime_error
{
public:
	UnknownChannelIdError (std::string s);
};


class NoReelsError : public std::runtime_error
{
public:
	NoReelsError ();
};


}


#endif
