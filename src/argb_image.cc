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

/** @file  src/argb_image.cc
 *  @brief ARGBImage class. 
 */

#include "argb_image.h"

using namespace dcp;

/** Construct an empty ARGBImage of a given size and with
 *  undefined contents.
 *  @param size Size in pixels.
 */
ARGBImage::ARGBImage (Size size)
	: _size (size)
{
	_data = new uint8_t[_size.width * _size.height * 4];
}


ARGBImage::~ARGBImage ()
{
	delete[] _data;
}

/** @return The stride, in bytes; that is, the number of bytes per row of the image */
int
ARGBImage::stride () const
{
	return _size.width * 4;
}
