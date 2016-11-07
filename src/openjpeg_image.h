/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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
 *  @brief OpenJPEGImage class.
 */

#include "util.h"

struct opj_image;
typedef struct opj_image opj_image_t;

namespace dcp {

/** @class OpenJPEGImage
 *  @brief A wrapper of libopenjpeg's opj_image_t.
 */
class OpenJPEGImage
{
public:
	explicit OpenJPEGImage (opj_image_t *);
	explicit OpenJPEGImage (OpenJPEGImage const & other);
	explicit OpenJPEGImage (Size);
	~OpenJPEGImage ();

	int* data (int) const;
	Size size () const;
	int precision (int component) const;
	bool srgb () const;

	/** @return Pointer to opj_image_t struct.  The caller
	 *  must not delete this.
	 */
	opj_image_t* opj_image () const {
		return _opj_image;
	}

private:
	opj_image_t* _opj_image; ///< opj_image_t that we are managing
};

}
