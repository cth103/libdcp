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

#ifndef LIBDCP_UTIL_H
#define LIBDCP_UTIL_H

/** @file  src/util.h
 *  @brief Utility methods.
 */

#include "types.h"
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <openjpeg.h>
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

extern bool operator== (Size const & a, Size const & b);
extern bool operator!= (Size const & a, Size const & b);
extern std::ostream& operator<< (std::ostream& s, Size const & a);

extern std::string make_uuid ();
extern std::string make_digest (boost::filesystem::path filename, boost::function<void (float)>);
extern std::string content_kind_to_string (ContentKind kind);
extern ContentKind content_kind_from_string (std::string kind);
extern bool empty_or_white_space (std::string s);
extern boost::shared_ptr<OpenJPEGImage> decompress_j2k (uint8_t* data, int64_t size, int reduce);
extern bool ids_equal (std::string a, std::string b);

extern void init ();

extern int base64_decode (std::string const & in, unsigned char* out, int out_length);
extern boost::optional<boost::filesystem::path> relative_to_root (boost::filesystem::path root, boost::filesystem::path file);
extern FILE * fopen_boost (boost::filesystem::path, std::string);
extern std::string file_to_string (boost::filesystem::path, uintmax_t max_length = 65536);
extern std::string private_key_fingerprint (std::string key);
extern xmlpp::Node* find_child (xmlpp::Node const * node, std::string name);

template <class F, class T>
std::list<boost::shared_ptr<T> >
list_of_type (std::list<boost::shared_ptr<F> > const & from)
{
	std::list<boost::shared_ptr<T> > out;
	for (typename std::list<boost::shared_ptr<F> >::const_iterator i = from.begin(); i != from.end(); ++i) {
		boost::shared_ptr<T> check = boost::dynamic_pointer_cast<T> (*i);
		if (check) {
			out.push_back (check);
		}
	}

	return out;
}

}

#endif
