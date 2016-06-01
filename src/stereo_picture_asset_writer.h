/*
    Copyright (C) 2012-2016 Carl Hetherington <cth@carlh.net>

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

#include "picture_asset_writer.h"
#include "types.h"
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <stdint.h>
#include <string>
#include <fstream>

namespace dcp {

/** @class StereoPictureAssetWriter
 *  @brief A helper class for writing to StereoPictureAssets.
 *
 *  Objects of this class can only be created with StereoPictureAsset::start_write().
 *
 *  Frames can be written to the StereoPictureAsset by calling write() with a JPEG2000 image
 *  (a verbatim .j2c file).  finalize() must be called after the last frame has been written.
 *  The action of finalize() can't be done in StereoPictureAssetWriter's destructor as it may
 *  throw an exception.
 */
class StereoPictureAssetWriter : public PictureAssetWriter
{
public:
	/** Write a frame for one eye.  Frames must be written left, then right, then left etc.
	 *  @param data JPEG2000 data.
	 *  @param size Size of data.
	 */
	FrameInfo write (uint8_t* data, int size);
	void fake_write (int size);
	bool finalize ();

private:
	friend class StereoPictureAsset;

	StereoPictureAssetWriter (PictureAsset *, boost::filesystem::path file, Standard, bool);
	void start (uint8_t *, int);

	/* do this with an opaque pointer so we don't have to include
	   ASDCP headers
	*/

	struct ASDCPState;
	boost::shared_ptr<ASDCPState> _state;

	dcp::Eye _next_eye;
};

}
