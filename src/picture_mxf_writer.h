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

/** @file  src/picture_mxf_writer.h
 *  @brief PictureMXFWriter and FrameInfo classes.
 */

#ifndef LIBDCP_PICTURE_MXF_WRITER_H
#define LIBDCP_PICTURE_MXF_WRITER_H

#include "metadata.h"
#include "types.h"
#include "mxf_writer.h"
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <stdint.h>
#include <string>
#include <fstream>

namespace dcp {

class PictureMXF;	

/** @class FrameInfo
 *  @brief Information about a single frame (either a monoscopic frame or a left *or* right eye stereoscopic frame)
 */
struct FrameInfo
{
	FrameInfo (uint64_t o, uint64_t s, std::string h)
		: offset (o)
		, size (s)
		, hash (h)
	{}

	FrameInfo (std::istream& s);
	FrameInfo (FILE *);

	void write (std::ostream& s) const;
	void write (FILE *) const;
	
	uint64_t offset;
	uint64_t size;
	std::string hash;
};

/** @class PictureMXFWriter
 *  @brief Parent class for classes which write picture MXF files.
 */
class PictureMXFWriter : public MXFWriter
{
public:
	virtual FrameInfo write (uint8_t *, int) = 0;
	virtual void fake_write (int) = 0;

protected:
	template <class P, class Q>
	friend void start (PictureMXFWriter *, boost::shared_ptr<P>, Standard, Q *, uint8_t *, int);

	PictureMXFWriter (PictureMXF *, boost::filesystem::path, Standard standard, bool);

	PictureMXF* _picture_mxf;
	bool _started;
	Standard _standard;
	bool _overwrite;
};

}

#endif
