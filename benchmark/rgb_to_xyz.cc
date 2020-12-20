/*
    Copyright (C) 2020 Carl Hetherington <cth@carlh.net>

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

#include "openjpeg_image.h"
#include "rgb_xyz.h"
#include "colour_conversion.h"
#include <boost/scoped_array.hpp>
#include <stdint.h>

using boost::scoped_array;
using std::shared_ptr;

int const trials = 256;

int
main ()
{
	srand (1);

	dcp::Size size(1998, 1080);

	scoped_array<uint8_t> rgb (new uint8_t[size.width * size.height * 6]);
	for (int y = 0; y < size.height; ++y) {
		uint16_t* p = reinterpret_cast<uint16_t*> (rgb.get() + y * size.width * 6);
		for (int x = 0; x < size.width; ++x) {
			for (int c = 0; c < 3; ++c) {
				*p = (rand() & 0xfff) << 4;
				++p;
			}
		}
	}

	shared_ptr<dcp::OpenJPEGImage> xyz;
	for (int i = 0; i < trials; ++i) {
		xyz = dcp::rgb_to_xyz (rgb.get(), size, size.width * 6, dcp::ColourConversion::srgb_to_xyz());
	}
}


