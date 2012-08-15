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

#include "rgba_frame.h"

using namespace libdcp;

RGBAFrame::RGBAFrame (int width, int height)
	: _width (width)
	, _height (height)
{
	_data = new uint8_t[width * height * 4];
}


RGBAFrame::~RGBAFrame ()
{
	delete[] _data;
}

int
RGBAFrame::stride () const
{
	return _width * 4;
}
