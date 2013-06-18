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

#include "argb_frame.h"

using namespace libdcp;

/** Construct an empty ARGBFrame of a given size and with
 *  undefined contents.
 *  @param size Size in pixels.
 */
ARGBFrame::ARGBFrame (Size size)
	: _size (size)
{
	_data = new uint8_t[_size.width * _size.height * 4];
}


ARGBFrame::~ARGBFrame ()
{
	delete[] _data;
}

/** @return The stride, in bytes; that is, the number of bytes per row of the image */
int
ARGBFrame::stride () const
{
	return _size.width * 4;
}
