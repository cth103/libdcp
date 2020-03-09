/*
    Copyright (C) 2018-2020 Carl Hetherington <cth@carlh.net>

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
		VERIFY_WARNING
	};

	enum Code {
		/** An error when reading the DCP.  note contains (probably technical) details. */
		GENERAL_READ,
		/** The hash of the CPL in the PKL does not agree with the CPL file */
		CPL_HASH_INCORRECT,
		/** Frame rate given in a reel for the main picture is not 24, 25, 30, 48, 50 or 60 */
		INVALID_PICTURE_FRAME_RATE,
		/** The hash of a main picture asset does not agree with the PKL file.  file contains the picture asset filename. */
		PICTURE_HASH_INCORRECT,
		/** The hash of a main picture is different in the CPL and PKL */
		PKL_CPL_PICTURE_HASHES_DISAGREE,
		/** The hash of a main sound asset does not agree with the PKL file.  file contains the sound asset filename. */
		SOUND_HASH_INCORRECT,
		/** The hash of a main sound is different in the CPL and PKL */
		PKL_CPL_SOUND_HASHES_DISAGREE,
		/** An assetmap's <Path> entry is empty */
		EMPTY_ASSET_PATH,
		/** An file mentioned in an asset map cannot be found */
		MISSING_ASSET,
		/** The DCP contains both SMPTE and Interop-standard components */
		MISMATCHED_STANDARD,
		/** Some XML fails to validate against the XSD/DTD */
		XML_VALIDATION_ERROR,
		/** No ASSETMAP{.xml} was found */
		MISSING_ASSETMAP,
		/** An asset's IntrinsicDuration is less than 1 second */
		INTRINSIC_DURATION_TOO_SMALL,
		/** An asset's Duration is less than 1 second */
		DURATION_TOO_SMALL
	};

	VerificationNote (Type type, Code code)
		: _type (type)
		, _code (code)
	{}

	VerificationNote (Type type, Code code, std::string note)
		: _type (type)
		, _code (code)
		, _note (note)
	{}

	VerificationNote (Type type, Code code, boost::filesystem::path file)
		: _type (type)
		, _code (code)
		, _file (file)
	{}

	VerificationNote (Type type, Code code, std::string note, boost::filesystem::path file)
		: _type (type)
		, _code (code)
		, _note (note)
		, _file (file)
	{}

	VerificationNote (Type type, Code code, std::string note, boost::filesystem::path file, uint64_t line)
		: _type (type)
		, _code (code)
		, _note (note)
		, _file (file)
		, _line (line)
	{}

	Type type () const {
		return _type;
	}

	Code code () const {
		return _code;
	}

	boost::optional<std::string> note () const {
		return _note;
	}

	boost::optional<boost::filesystem::path> file () const {
		return _file;
	}

	boost::optional<uint64_t> line () const {
		return _line;
	}

private:
	Type _type;
	Code _code;
	/** Further information about the error, if applicable */
	boost::optional<std::string> _note;
	/** Path of file containing the error, if applicable */
	boost::optional<boost::filesystem::path> _file;
	/** Error line number within _file, if applicable */
	uint64_t _line;
};

std::list<VerificationNote> verify (
	std::vector<boost::filesystem::path> directories,
	boost::function<void (std::string, boost::optional<boost::filesystem::path>)> stage,
	boost::function<void (float)> progress,
	boost::filesystem::path xsd_dtd_directory
	);

std::string note_to_string (dcp::VerificationNote note);

}

#endif
