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

/** @file  src/mono_picture_mxf_writer.h
 *  @brief MonoPictureMXFWriter class
 */

#ifndef LIBDCP_MONO_PICTURE_MXF_WRITER_H
#define LIBDCP_MONO_PICTURE_MXF_WRITER_H

#include "picture_mxf_writer.h"
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <stdint.h>
#include <string>
#include <fstream>

namespace dcp {

/** @class MonoPictureMXFWriter
 *  @brief A helper class for writing to MonoPictureMXFs
 *
 *  Objects of this class can only be created with MonoPictureMXF::start_write().
 *
 *  Frames can be written to the MonoPictureMXF by calling write() with a JPEG2000 image
 *  (a verbatim .j2c file).  finalize() must be called after the last frame has been written.
 *  The action of finalize() can't be done in MonoPictureAssetWriter's destructor as it may
 *  throw an exception.
 */
class MonoPictureMXFWriter : public PictureMXFWriter
{
public:
	FrameInfo write (uint8_t *, int);
	void fake_write (int size);
	void finalize ();

private:
	friend class MonoPictureMXF;

	MonoPictureMXFWriter (PictureMXF *, boost::filesystem::path file, Standard standard, bool);
	void start (uint8_t *, int);

	/* do this with an opaque pointer so we don't have to include
	   ASDCP headers
	*/
	   
	struct ASDCPState;
	boost::shared_ptr<ASDCPState> _state;
};

}

#endif
