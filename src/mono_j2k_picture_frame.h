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


/** @file  src/mono_j2k_picture_frame.h
 *  @brief MonoJ2KPictureFrame class
 */


#ifndef LIBDCP_MONO_J2K_PICTURE_FRAME_H
#define LIBDCP_MONO_J2K_PICTURE_FRAME_H


#include "asset_reader.h"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <memory>
#include <stdint.h>
#include <string>


namespace ASDCP {
	namespace JP2K {
		class FrameBuffer;
		class MXFReader;
	}
	class AESDecContext;
}


namespace dcp {


class OpenJPEGImage;


/** @class MonoJ2KPictureFrame
 *  @brief A single frame of a 2D (monoscopic) picture asset
 */
class MonoJ2KPictureFrame : public Data
{
public:
	/** Make a picture frame from a JPEG2000 file.
	 *  @param path Path to JPEG2000 file.
	 */
	explicit MonoJ2KPictureFrame (boost::filesystem::path path);
	MonoJ2KPictureFrame (uint8_t const * data, int size);

	MonoJ2KPictureFrame (MonoJ2KPictureFrame const&) = delete;
	MonoJ2KPictureFrame& operator= (MonoJ2KPictureFrame const&) = delete;

	/** @param reduce a factor by which to reduce the resolution
	 *  of the image, expressed as a power of two (pass 0 for no
	 *  reduction).
	 */
	std::shared_ptr<OpenJPEGImage> xyz_image (int reduce = 0) const;

	/** @return Pointer to JPEG2000 data */
	uint8_t const * data () const override;

	/** @return Pointer to JPEG2000 data */
	uint8_t* data () override;

	/** @return Size of JPEG2000 data in bytes */
	int size () const override;

private:
	/* XXX: this is a bit of a shame, but I tried friend MonoJ2KPictureAssetReader and it's
	   rejected by some (seemingly older) GCCs.
	*/
	friend class AssetReader<ASDCP::JP2K::MXFReader, MonoJ2KPictureFrame>;

	MonoJ2KPictureFrame (ASDCP::JP2K::MXFReader* reader, int n, std::shared_ptr<DecryptionContext>, bool check_hmac);

	std::shared_ptr<ASDCP::JP2K::FrameBuffer> _buffer;
};

}

#endif
