/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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
*/

/** @file  src/sound_frame.h
 *  @brief SoundFrame class.
 */

#ifndef LIBDCP_SOUND_FRAME_H
#define LIBDCP_SOUND_FRAME_H

#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>
#include <stdint.h>
#include <string>

namespace ASDCP {
	namespace PCM {
		class FrameBuffer;
		class MXFReader;
	}
	class AESDecContext;
}

namespace dcp {

/** @class SoundFrame
 *  @brief One &lsquo;frame&rsquo; of sound data from a SoundAsset.
 */
class SoundFrame : public boost::noncopyable
{
public:
	~SoundFrame ();

	uint8_t const * data () const;
	int size () const;

private:
	friend class SoundAssetReader;

	SoundFrame (ASDCP::PCM::MXFReader* reader, int n, ASDCP::AESDecContext *);

	/** a buffer to hold the frame */
	ASDCP::PCM::FrameBuffer* _buffer;
};

}

#endif
