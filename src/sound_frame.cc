/*
    Copyright (C) 2012-2016 Carl Hetherington <cth@carlh.net>

    This file is part of libdcp.

    libdcp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libdcp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdcp.  If not, see <http://www.gnu.org/licenses/>.

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/

/** @file  src/sound_frame.cc
 *  @brief SoundFrame class.
 */

#include "sound_frame.h"
#include "exceptions.h"
#include "AS_DCP.h"
#include "KM_fileio.h"

using namespace std;
using namespace dcp;

SoundFrame::SoundFrame (ASDCP::PCM::MXFReader* reader, int n, ASDCP::AESDecContext* c)
{
	/* XXX: unfortunate guesswork on this buffer size */
	_buffer = new ASDCP::PCM::FrameBuffer (1 * Kumu::Megabyte);

	if (ASDCP_FAILURE (reader->ReadFrame (n, *_buffer, c))) {
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
