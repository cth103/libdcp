/*
    Copyright (C) 2014-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/exceptions.cc
 *  @brief Exceptions thrown by libdcp
 */


#include "exceptions.h"
#include "compose.hpp"


using std::string;
using std::runtime_error;
using boost::optional;
using namespace dcp;


FileError::FileError (string message, boost::filesystem::path filename, int number)
	: runtime_error (String::compose ("%1 (%2) (error %3)", message, filename.string(), number))
	, _filename (filename)
	, _number (number)
{

}


UnresolvedRefError::UnresolvedRefError (string id)
	: runtime_error (String::compose ("Unresolved reference to asset id %1", id))
{

}


TimeFormatError::TimeFormatError (string bad_time)
	: runtime_error (String::compose ("Bad time string %1", bad_time))
{

}


BadContentKindError::BadContentKindError (string content_kind)
	: ReadError (String::compose("Bad content kind '%1'", content_kind))
{

}


NotEncryptedError::NotEncryptedError (string const & what)
	: runtime_error (String::compose ("%1 is not encrypted", what))
{

}


ProgrammingError::ProgrammingError (string file, int line)
	: runtime_error (String::compose ("Programming error at %1:%2", file, line))
{

}


KDMDecryptionError::KDMDecryptionError (std::string message, int cipher_length, int modulus_dmax)
	: runtime_error (String::compose ("Could not decrypt KDM (%1) (%2/%3)", message, cipher_length, modulus_dmax))
{

}


KDMFormatError::KDMFormatError (std::string message)
	: runtime_error (String::compose ("Could not parse KDM (%1)", message))
{

}


CertificateChainError::CertificateChainError (string message)
	: runtime_error (message)
{

}


ReadError::ReadError (string message, string detail)
	: runtime_error(String::compose("%1 (%2)", message, detail))
	, _message(message)
	, _detail(detail)
{

}


MissingSubtitleImageError::MissingSubtitleImageError (string id)
	: runtime_error (String::compose("Could not load image for subtitle %1", id))
{

}


BadKDMDateError::BadKDMDateError (bool starts_too_early)
	: runtime_error (
		starts_too_early ?
		"KDM validity period starts before or close to the start of the signing certificate validity period" :
		"KDM validity ends after or close to the end of the signing certificate's validity period"
		)
	, _starts_too_early (starts_too_early)
{

}


StartCompressionError::StartCompressionError (optional<int> code)
	: runtime_error (String::compose("Could not start JPEG2000 encoding%1", code ? String::compose(" (%1", *code) : ""))
	, _code (code)
{}



CombineError::CombineError (string message)
	: runtime_error (message)
{}


LanguageTagError::LanguageTagError (std::string message)
	: runtime_error (message)
{}


BadSettingError::BadSettingError (std::string message)
	: runtime_error (message)
{

}


DuplicateIdError::DuplicateIdError (std::string message)
	: runtime_error (message)
{

}


MainSoundConfigurationError::MainSoundConfigurationError (std::string s)
	: runtime_error (String::compose("Could not parse MainSoundConfiguration %1", s))
{

}


UnknownChannelIdError::UnknownChannelIdError (std::string id)
	: runtime_error (String::compose("Unrecognised channel id '%1'", id))
{

}


NoReelsError::NoReelsError ()
	: runtime_error ("Cannot make a DCP when no reels have been added")
{

}


MissingAssetmapError::MissingAssetmapError (boost::filesystem::path dir)
	: ReadError (String::compose("Could not find ASSETMAP nor ASSETMAP.xml in '%1'", dir.string()))
{

}

