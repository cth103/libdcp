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

/** @file  src/mono_picture_frame.h
 *  @brief MonoPictureFrame class.
 */

#include "types.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <string>
#include <stdint.h>

namespace ASDCP {
	namespace JP2K {
		class FrameBuffer;
	}
	class AESDecContext;
}

namespace dcp {

class ARGBImage;
class Image;

/** @class MonoPictureFrame
 *  @brief A single frame of a 2D (monoscopic) picture asset.
 */
class MonoPictureFrame : public boost::noncopyable
{
public:
	MonoPictureFrame (boost::filesystem::path mxf_path, int n, ASDCP::AESDecContext *);
	MonoPictureFrame (boost::filesystem::path path);
	MonoPictureFrame ();
	~MonoPictureFrame ();

	boost::shared_ptr<ARGBImage> argb_image (int reduce = 0) const;
	void rgb_frame (boost::shared_ptr<Image> rgb, boost::optional<NoteHandler> note = boost::optional<NoteHandler> ()) const;
	uint8_t const * j2k_data () const;
	uint8_t* j2k_data ();
	int j2k_size () const;

private:
	ASDCP::JP2K::FrameBuffer* _buffer;
};

}
