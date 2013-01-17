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

#ifndef LIBDCP_UTIL_H
#define LIBDCP_UTIL_H

/** @file  src/util.h
 *  @brief Utility methods.
 */

#include <string>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <openjpeg.h>
#include "types.h"

namespace libdcp {

class ARGBFrame;

struct Size {
	Size ()
		: width (0)
		, height (0)
	{}

	Size (int w, int h)
		: width (w)
		, height (h)
	{}
	
	int width;
	int height;
};
	
extern bool operator== (Size const & a, Size const & b);
extern bool operator!= (Size const & a, Size const & b);

extern std::string make_uuid ();
extern std::string make_digest (std::string filename);
extern std::string content_kind_to_string (ContentKind kind);
extern ContentKind content_kind_from_string (std::string kind);
extern bool empty_or_white_space (std::string s);
extern opj_image_t* decompress_j2k (uint8_t* data, int64_t size, int reduce);
extern boost::shared_ptr<ARGBFrame> xyz_to_rgb (opj_image_t* xyz_frame);

}

#endif
