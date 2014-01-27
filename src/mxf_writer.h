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

/** @file  src/mxf_writer.h
 *  @brief MXFWriter class.
 */

#ifndef LIBDCP_MXF_WRITER_H
#define LIBDCP_MXF_WRITER_H

#include <boost/filesystem.hpp>

namespace dcp {

class MXF;

/** @class MXFWriter
 *  @brief Parent class for classes which can write MXF files.
 *
 *  The MXFWriter lasts for the duration of the write and is then discarded.
 *  They can only be created by calling start_write() on an MXF object.
 */
class MXFWriter : public boost::noncopyable
{
public:
	virtual ~MXFWriter ();
	virtual void finalize ();

protected:
	MXFWriter (MXF* mxf, boost::filesystem::path file);

	/** MXF that we are writing */
	MXF* _mxf;
	/** File that we are writing to */
	boost::filesystem::path _file;
	/** Number of `frames' written so far; the definition of a frame
	 *  varies depending on the subclass.
	 */
	int64_t _frames_written;
	/** true if finalize() has been called on this object */
	bool _finalized;
};

}

#endif
