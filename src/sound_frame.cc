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

#include "AS_DCP.h"
#include "KM_fileio.h"
#include "sound_frame.h"
#include "exceptions.h"

using namespace std;
using namespace libdcp;

SoundFrame::SoundFrame (string mxf_path, int n, ASDCP::AESDecContext* c)
{
	ASDCP::PCM::MXFReader reader;
	Kumu::Result_t r = reader.OpenRead (mxf_path.c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (FileError ("could not open MXF file for reading", mxf_path, r));
	}

	/* XXX: unfortunate guesswork on this buffer size */
	_buffer = new ASDCP::PCM::FrameBuffer (1 * Kumu::Megabyte);

	if (ASDCP_FAILURE (reader.ReadFrame (n, *_buffer, c))) {
		boost::throw_exception (DCPReadError ("could not read audio frame"));
	}
}

SoundFrame::~SoundFrame ()
{
	delete _buffer;
}

uint8_t const *
SoundFrame::data () const
{
	return _buffer->RoData();
}

int
SoundFrame::size () const
{
	return _buffer->Size ();
}
