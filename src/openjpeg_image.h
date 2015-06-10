/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

/** @file  src/openjpeg_image.h
 *  @brief OpenJPEGImage class.
 */

#include "util.h"
#include <openjpeg.h>

namespace dcp {

/** @class OpenJPEGImage
 *  @brief A wrapper of libopenjpeg's opj_image_t.
 */
class OpenJPEGImage : public boost::noncopyable
{
public:
	OpenJPEGImage (opj_image_t *);
	OpenJPEGImage (Size);
	~OpenJPEGImage ();

	int* data (int) const;
	dcp::Size size () const;

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
