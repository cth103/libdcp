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

#include "types.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>
#include <stdint.h>
#include <string>

namespace ASDCP {
	namespace JP2K {
		struct SFrameBuffer;
	}
	class AESDecContext;
}

namespace dcp {

class ARGBFrame;

/** A single frame of a 3D (stereoscopic) picture asset */	
class StereoPictureFrame : public boost::noncopyable
{
public:
	StereoPictureFrame (boost::filesystem::path mxf_path, int n);
	StereoPictureFrame ();
	~StereoPictureFrame ();

	boost::shared_ptr<ARGBFrame> argb_frame (Eye eye, int reduce = 0, float srgb_gamma = 2.4) const;
	void rgb_frame (Eye eye, uint16_t* buffer) const;
	uint8_t const * left_j2k_data () const;
	uint8_t* left_j2k_data ();
	int left_j2k_size () const;
	uint8_t const * right_j2k_data () const;
	uint8_t* right_j2k_data ();
	int right_j2k_size () const;

private:
	ASDCP::JP2K::SFrameBuffer* _buffer;
};

}
