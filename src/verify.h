/*
    Copyright (C) 2018-2021 Carl Hetherington <cth@carlh.net>

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
		VERIFY_BV21_ERROR, ///< may not always be considered an error, but violates a "shall" requirement of Bv2.1
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
		PKL_CPL_PICTURE_HASHES_DIFFER,
		/** The hash of a main sound asset does not agree with the PKL file.  file contains the sound asset filename. */
		SOUND_HASH_INCORRECT,
		/** The hash of a main sound is different in the CPL and PKL */
		PKL_CPL_SOUND_HASHES_DIFFER,
		/** An assetmap's <Path> entry is empty */
		EMPTY_ASSET_PATH,
		/** A file mentioned in an asset map cannot be found */
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
		DURATION_TOO_SMALL,
		/** The JPEG2000 data in at least one picture frame is larger than the equivalent of 250Mbit/s */
		PICTURE_FRAME_TOO_LARGE_IN_BYTES,
		/** The JPEG2000 data in at least one picture frame is larger than the equivalent of 230Mbit/s */
		PICTURE_FRAME_NEARLY_TOO_LARGE_IN_BYTES,
		/** An asset that the CPL requires is not in this DCP; the DCP may be a VF */
		EXTERNAL_ASSET,
		/** DCP is Interop, not SMPTE [Bv2.1_6.1] */
		NOT_SMPTE,
		/** A language or territory does not conform to RFC 5646 [Bv2.1_6.2.1] */
		BAD_LANGUAGE,
		/** A picture asset does not have one of the required Bv2.1 sizes (in pixels) [Bv2.1_7.1] */
		PICTURE_ASSET_INVALID_SIZE_IN_PIXELS,
		/** A picture asset is 2K but is not at 24, 25 or 48 fps as required by Bv2.1 [Bv2.1_7.1] */
		PICTURE_ASSET_INVALID_FRAME_RATE_FOR_2K,
		/** A picture asset is 4K but is not at 24fps as required by Bv2.1 [Bv2.1_7.1] */
		PICTURE_ASSET_INVALID_FRAME_RATE_FOR_4K,
		/** A picture asset is 4K but is 3D which is not allowed by Bv2.1 [Bv2.1_7.1] */
		PICTURE_ASSET_4K_3D,
		/** A closed caption's XML file is larger than 256KB [Bv2.1_7.2.1] */
		CLOSED_CAPTION_XML_TOO_LARGE_IN_BYTES,
		/** Any timed text asset's total files is larger than 115MB [Bv2.1_7.2.1] */
		TIMED_TEXT_ASSET_TOO_LARGE_IN_BYTES,
		/** The total size of all a timed text asset's fonts is larger than 10MB [Bv2.1_7.2.1] */
		TIMED_TEXT_FONTS_TOO_LARGE_IN_BYTES,
		/** Some SMPTE subtitle XML has no <Language> tag [Bv2.1_7.2.2] */
		MISSING_SUBTITLE_LANGUAGE,
		/** Not all subtitle assets specify the same <Language> tag [Bv2.1_7.2.2] */
		SUBTITLE_LANGUAGES_DIFFER,
		/** Some SMPTE subtitle XML has no <StartTime> tag [Bv2.1_7.2.3] */
		MISSING_SUBTITLE_START_TIME,
		/** Some SMPTE subtitle XML has a non-zero <StartTime> tag [Bv2.1_7.2.3] */
		SUBTITLE_START_TIME_NON_ZERO,
		/** The first subtitle or closed caption happens before 4s into the first reel [Bv2.1_7.2.4] */
		FIRST_TEXT_TOO_EARLY,
		/** At least one subtitle is less than the minimum of 15 frames suggested by [Bv2.1_7.2.5] */
		SUBTITLE_TOO_SHORT,
		/** At least one pair of subtitles are separated by less than the the minimum of 2 frames suggested by [Bv2.1_7.2.5] */
		SUBTITLE_TOO_CLOSE,
		/** There are more than 3 subtitle lines in at least one place [Bv2.1_7.2.7] */
		TOO_MANY_SUBTITLE_LINES,
		/** There are more than 52 characters in at least one subtitle line [Bv2.1_7.2.7] */
		SUBTITLE_LINE_LONGER_THAN_RECOMMENDED,
		/** There are more than 79 characters in at least one subtitle line [Bv2.1_7.2.7] */
		SUBTITLE_LINE_TOO_LONG,
		/** There are more than 3 closed caption lines in at least one place [Bv2.1_7.2.6] */
		TOO_MANY_CLOSED_CAPTION_LINES,
		/** There are more than 32 characters in at least one closed caption line [Bv2.1_7.2.6] */
		CLOSED_CAPTION_LINE_TOO_LONG,
		/** The audio sampling rate must be 48kHz [Bv2.1_7.3] */
		INVALID_SOUND_FRAME_RATE,
		/** The CPL has no <AnnotationText> tag [Bv2.1_8.1] */
		MISSING_ANNOTATION_TEXT_IN_CPL,
		/** The <AnnotationText> is not the same as the <ContentTitleText> [Bv2.1_8.1] */
		CPL_ANNOTATION_TEXT_DIFFERS_FROM_CONTENT_TITLE_TEXT,
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

std::vector<VerificationNote> verify (
	std::vector<boost::filesystem::path> directories,
	boost::function<void (std::string, boost::optional<boost::filesystem::path>)> stage,
	boost::function<void (float)> progress,
	boost::filesystem::path xsd_dtd_directory
	);

std::string note_to_string (dcp::VerificationNote note);

}

#endif
