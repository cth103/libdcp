/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_UTIL_H
#define LIBDCP_UTIL_H

/** @file  src/util.h
 *  @brief Utility methods.
 */

#include "types.h"
#include "data.h"
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <string>
#include <stdint.h>

namespace xmlpp {
	class Element;
	class Node;
}

namespace dcp {

class ARGBImage;
class CertificateChain;
class GammaLUT;
class OpenJPEGImage;

extern std::string make_uuid ();
extern std::string make_digest (boost::filesystem::path filename, boost::function<void (float)>);
extern std::string content_kind_to_string (ContentKind kind);
extern ContentKind content_kind_from_string (std::string kind);
extern bool empty_or_white_space (std::string s);
extern bool ids_equal (std::string a, std::string b);
extern std::string remove_urn_uuid (std::string raw);
extern void init ();

extern int base64_decode (std::string const & in, unsigned char* out, int out_length);
extern boost::optional<boost::filesystem::path> relative_to_root (boost::filesystem::path root, boost::filesystem::path file);
extern FILE * fopen_boost (boost::filesystem::path, std::string);
extern std::string file_to_string (boost::filesystem::path, uintmax_t max_length = 65536);
extern std::string private_key_fingerprint (std::string key);
extern xmlpp::Node* find_child (xmlpp::Node const * node, std::string name);

}

#endif
