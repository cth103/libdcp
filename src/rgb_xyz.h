/*
    Copyright (C) 2013-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  rgb_xyz.h
 *  @brief Conversion between RGB and XYZ
 */


#include "types.h"
#include <memory>
#include <boost/optional.hpp>
#include <stdint.h>


namespace dcp {


class OpenJPEGImage;
class Image;
class ColourConversion;


/** Convert an XYZ image to RGBA.
 *  @param xyz_image Image in XYZ.
 *  @param conversion Colour conversion to use.
 *  @param argb Buffer to fill with RGBA data.  The format of the data is:
 *
 *  <pre>
 *  Byte   /- 0 -------|- 1 --------|- 2 --------|- 3 --------|- 4 --------|- 5 --------| ...
 *         |(0, 0) Blue|(0, 0)Green |(0, 0) Red  |(0, 0) Alpha|(0, 1) Blue |(0, 1) Green| ...
 *  </pre>
 *
 *  So that the first byte is the blue component of the pixel at x=0, y=0, the second
 *  is the green component, and so on.
 *
 *  Lines are packed so that the second row directly follows the first.
 */
extern void xyz_to_rgba (
	std::shared_ptr<const OpenJPEGImage>,
	ColourConversion const & conversion,
	uint8_t* rgba,
	int stride
	);


/** Convert an XYZ image to 48bpp RGB.
 *  @param xyz_image Frame in XYZ.
 *  @param conversion Colour conversion to use.
 *  @param rgb Buffer to fill with RGB data.  Format is packed RGB
 *  16:16:16, 48bpp, 16R, 16G, 16B, with the 2-byte value for each
 *  R/G/B component stored as little-endian; i.e. AV_PIX_FMT_RGB48LE.
 *  @param stride Stride for RGB data in bytes.
 *  @param note Optional handler for any notes that may be made during the conversion (e.g. when clamping occurs).
 */
extern void xyz_to_rgb (
	std::shared_ptr<const OpenJPEGImage>,
	ColourConversion const & conversion,
	uint8_t* rgb,
	int stride,
	boost::optional<NoteHandler> note = boost::optional<NoteHandler> ()
	);


/** @param rgb RGB data; packed RGB 16:16:16, 48bpp, 16R, 16G, 16B,
 *  with the 2-byte value for each R/G/B component stored as
 *  little-endian; i.e. AV_PIX_FMT_RGB48LE.
 *  @param size size of RGB image in pixels.
 *  @param size stride of RGB data in pixels.
 */
extern std::shared_ptr<OpenJPEGImage> rgb_to_xyz (
	uint8_t const * rgb,
	dcp::Size size,
	int stride,
	ColourConversion const & conversion,
	boost::optional<NoteHandler> note = boost::optional<NoteHandler> ()
	);


/** @param conversion Colour conversion.
 *  @param matrix Filled in with the product of the RGB to XYZ matrix, the Bradford transform and the DCI companding.
 */
extern void combined_rgb_to_xyz (ColourConversion const & conversion, double* matrix);

}
