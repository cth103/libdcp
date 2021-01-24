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


/** @file  src/openjpeg_image.h
 *  @brief OpenJPEGImage class
 */


#include "util.h"


struct opj_image;
typedef struct opj_image opj_image_t;


namespace dcp {


/** @class OpenJPEGImage
 *  @brief A wrapper of libopenjpeg's opj_image_t
 */
class OpenJPEGImage
{
public:
	/** Construct an OpenJPEGImage, taking ownership of the opj_image_t */
	explicit OpenJPEGImage (opj_image_t *);

	explicit OpenJPEGImage (OpenJPEGImage const & other);

	/** Construct a new OpenJPEGImage with undefined contents
	 *  @param size Size for the frame in pixels
	 */
	explicit OpenJPEGImage (Size);

	/** @param data_16 XYZ/RGB image data in packed 16:16:16, 48bpp with
	 *  the 2-byte value for each component stored as little-endian
	 */
	OpenJPEGImage (uint8_t const * in_16, dcp::Size size, int stride);

	~OpenJPEGImage ();

	/** @param c Component index (0, 1 or 2)
	 *  @return Pointer to the data for component c.
	 */
	int* data (int) const;

	/** @return Size of the image in pixels */
	Size size () const;

	int precision (int component) const;
	bool srgb () const;
	int factor (int component) const;

	/** @return Pointer to opj_image_t struct.  The caller
	 *  must not delete this.
	 */
	opj_image_t* opj_image () const {
		return _opj_image;
	}

private:
	void create (Size size);

	opj_image_t* _opj_image = nullptr; ///< opj_image_t that we are managing
};


}
