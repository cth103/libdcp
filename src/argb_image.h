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

/** @file  src/argb_image.h
 *  @brief ARGBImage class. 
 */

#include "util.h"
#include <stdint.h>

namespace dcp
{

/** @class ARGBImage
 *  @brief A single frame of picture data held in an ARGB buffer.
 *
 *  The format of the data is:
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
 *
 *  XXX: this should probably be an Image...?
 */
class ARGBImage : boost::noncopyable
{
public:
	ARGBImage (Size size);
	~ARGBImage ();

	/** @return pointer to the image data */
	uint8_t* data () const {
		return _data;
	}

	/** @return length of one picture row in bytes */
	int stride () const;

	/** @return size of the picture in pixels */
	Size size () const {
		return _size;
	}

private:
	Size _size;     ///< frame size in pixels
	uint8_t* _data; ///< pointer to image data
};

}
