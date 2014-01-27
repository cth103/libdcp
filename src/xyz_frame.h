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

/** @file  src/xyz_frame.h
 *  @brief XZYFrame class.
 */

#include "util.h"
#include <openjpeg.h>

namespace dcp {

/* @class XYZFrame
 * @brief An image in XYZ colour.
 */
class XYZFrame : public boost::noncopyable
{
public:
	XYZFrame (opj_image_t *);
	XYZFrame (Size);
	~XYZFrame ();

	int* data (int) const;
	dcp::Size size () const;

	opj_image_t* opj_image () const {
		return _opj_image;
	}

private:
	opj_image_t* _opj_image;
};

}
