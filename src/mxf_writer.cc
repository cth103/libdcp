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

#include "mxf_writer.h"
#include "mxf.h"
#include "dcp_assert.h"

using namespace dcp;

/** Create an MXFWriter.
 *  @param mxf MXF that we are writing.
 *  @param file File to write to.
 */
MXFWriter::MXFWriter (MXF* mxf, boost::filesystem::path file)
	: _mxf (mxf)
	, _file (file)
	, _frames_written (0)
	, _finalized (false)
{
	mxf->set_file (file);
}

MXFWriter::~MXFWriter ()
{

}

void
MXFWriter::finalize ()
{
	DCP_ASSERT (!_finalized);
	_finalized = true;
}
