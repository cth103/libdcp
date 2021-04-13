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


/** @file  src/util.h
 *  @brief Utility methods and classes
 */


#ifndef LIBDCP_UTIL_H
#define LIBDCP_UTIL_H


#include "array_data.h"
#include "local_time.h"
#include "types.h"
LIBDCP_DISABLE_WARNINGS
#include <asdcp/KM_log.h>
LIBDCP_ENABLE_WARNINGS
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <memory>
#include <string>
#include <stdint.h>


#define LIBDCP_UNUSED(x) (void)(x)


namespace xmlpp {
	class Element;
	class Node;
}


namespace dcp {


class CertificateChain;
class GammaLUT;
class OpenJPEGImage;


extern std::string make_uuid ();

/** Create a digest for a file
 *  @param filename File name
 *  @param progress Optional progress reporting function.  The function will be called
 *  with a progress value between 0 and 1
 *  @return Digest
 */
extern std::string make_digest (boost::filesystem::path filename, boost::function<void (float)>);

extern std::string make_digest (ArrayData data);

/** @param s A string
 *  @return true if the string contains only space, newline or tab characters, or is empty
 */
extern bool empty_or_white_space (std::string s);

extern bool ids_equal (std::string a, std::string b);
extern std::string remove_urn_uuid (std::string raw);

/** Set up various bits that the library needs.  Should be called once
 *  by client applications.
 *
 *  @param tags_directory Path to a copy of the tags directory from the source code;
 *  if none is specified libdcp will look for a tags directory in the environment
 *  variable LIBDCP_RESOURCES or based on where the current executable is.
 */
extern void init (boost::optional<boost::filesystem::path> tags_directory = boost::optional<boost::filesystem::path>());

/** Decode a base64 string.  The base64 decode routine in KM_util.cpp
 *  gives different values to both this and the command-line base64
 *  for some inputs.  Not sure why.
 *
 *  @param in base64-encoded string
 *  @param out Output buffer
 *  @param out_length Length of output buffer
 *  @return Number of characters written to the output buffer
 */
extern int base64_decode (std::string const & in, unsigned char* out, int out_length);

extern boost::optional<boost::filesystem::path> relative_to_root (boost::filesystem::path root, boost::filesystem::path file);

/** @param p Path to open
 *  @param t mode flags, as for fopen(3)
 *  @return FILE pointer or 0 on error
 *
 *  Apparently there is no way to create an ofstream using a UTF-8
 *  filename under Windows.  We are hence reduced to using fopen
 *  with this wrapper.
 */
extern FILE * fopen_boost (boost::filesystem::path, std::string);

extern std::string file_to_string (boost::filesystem::path, uintmax_t max_length = 1048576);

/** @param key RSA private key in PEM format (optionally with -----BEGIN... / -----END...)
 *  @return SHA1 fingerprint of key
 */
extern std::string private_key_fingerprint (std::string key);
extern xmlpp::Node* find_child (xmlpp::Node const * node, std::string name);
extern std::string openjpeg_version();
extern std::string spaces (int n);
extern void indent (xmlpp::Element* element, int initial);

/** @return true if the day represented by \ref a is less than or
 *  equal to the one represented by \ref b, ignoring the time parts
 */
extern bool day_less_than_or_equal (LocalTime a, LocalTime b);

/** @return true if the day represented by \ref a is greater than or
 *  equal to the one represented by \ref b, ignoring the time parts
 */
extern bool day_greater_than_or_equal (LocalTime a, LocalTime b);

/** Try quite hard to find a string which starts with \ref base and is
 *  not in \ref existing
 */
extern std::string unique_string (std::vector<std::string> existing, std::string base);

extern ASDCP::Dictionary const* asdcp_smpte_dict;

extern boost::filesystem::path directory_containing_executable ();
extern boost::filesystem::path resources_directory ();


class ASDCPErrorSuspender
{
public:
	ASDCPErrorSuspender();
	~ASDCPErrorSuspender();

private:
	Kumu::LogEntryList _log;
	Kumu::ILogSink& _old;
	Kumu::EntryListLogSink* _sink;
};


}


#endif
