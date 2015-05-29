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

/** @file  src/sound_mxf_writer.h
 *  @brief SoundMXFWriter class.
 */

#include "mxf_writer.h"
#include "types.h"
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

namespace dcp {

class SoundFrame;
class SoundMXF;

/** @class SoundMXFWriter
 *  @brief A helper class for writing to SoundMXFs.
 *
 *  Objects of this class can only be created with SoundMXF::start_write().
 *
 *  Sound samples can be written to the SoundMXF by calling write() with
 *  a buffer of float values.  finalize() must be called after the last samples
 *  have been written.
 */
class SoundMXFWriter : public MXFWriter
{
public:
	void write (float const * const *, int);
	void finalize ();

private:
	friend class SoundMXF;

	SoundMXFWriter (SoundMXF *, boost::filesystem::path, Standard standard);

	void write_current_frame ();

	/* do this with an opaque pointer so we don't have to include
	   ASDCP headers
	*/
	   
	struct ASDCPState;
	boost::shared_ptr<ASDCPState> _state;

	SoundMXF* _sound_mxf;
	int _frame_buffer_offset;
};

}

