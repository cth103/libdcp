/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/stereo_picture_asset_writer.h
 *  @brief StereoPictureAssetWriter class
 */


#include "picture_asset_writer.h"
#include "types.h"
#include <memory>
#include <stdint.h>
#include <string>


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
	FrameInfo write (uint8_t const * data, int size) override;
	void fake_write (int size) override;
	bool finalize () override;

private:
	friend class StereoPictureAsset;

	StereoPictureAssetWriter (PictureAsset *, boost::filesystem::path file, bool);
	void start (uint8_t const *, int);

	/* do this with an opaque pointer so we don't have to include
	   ASDCP headers
	*/

	struct ASDCPState;
	std::shared_ptr<ASDCPState> _state;

	dcp::Eye _next_eye = Eye::LEFT;
};


}
