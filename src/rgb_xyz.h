/*
    Copyright (C) 2013-2015 Carl Hetherington <cth@carlh.net>

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
*/

#include "types.h"
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <stdint.h>

namespace dcp {

class ARGBImage;
class OpenJPEGImage;
class Image;
class ColourConversion;

extern void xyz_to_rgba (
	boost::shared_ptr<const OpenJPEGImage>,
	ColourConversion const & conversion,
	uint8_t* rgba
	);

extern void xyz_to_rgb (
	boost::shared_ptr<const OpenJPEGImage>,
	ColourConversion const & conversion,
	uint8_t* rgb,
	int stride,
	boost::optional<NoteHandler> note = boost::optional<NoteHandler> ()
	);

extern boost::shared_ptr<OpenJPEGImage> rgb_to_xyz (
	uint8_t const * rgb,
	dcp::Size size,
	int stride,
	ColourConversion const & conversion,
	boost::optional<NoteHandler> note = boost::optional<NoteHandler> ()
	);

extern boost::shared_ptr<OpenJPEGImage> xyz_to_xyz (uint8_t const * xyz, dcp::Size size, int stride);

}
