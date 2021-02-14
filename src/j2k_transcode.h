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


/** @file  src/j2k_transcode.h
 *  @brief Methods to encode and decode JPEG2000
 */


#include "array_data.h"
#include <memory>
#include <stdint.h>


namespace dcp {


class OpenJPEGImage;


/** Decompress a JPEG2000 image to a bitmap
 *  @param data JPEG2000 data
 *  @param size Size of data in bytes
 *  @param reduce A power of 2 by which to reduce the size of the decoded image;
 *  e.g. 0 reduces by (2^0 == 1), ie keeping the same size.
 *       1 reduces by (2^1 == 2), ie halving the size of the image.
 *  This is useful for scaling 4K DCP images down to 2K.
 *  @return OpenJPEGImage
 */
extern std::shared_ptr<OpenJPEGImage> decompress_j2k (uint8_t const * data, int64_t size, int reduce);

extern std::shared_ptr<OpenJPEGImage> decompress_j2k (Data const& data, int reduce);
extern std::shared_ptr<OpenJPEGImage> decompress_j2k (std::shared_ptr<const Data> data, int reduce);

/** @xyz Picture to compress.  Parts of xyz's data WILL BE OVERWRITTEN by libopenjpeg so xyz cannot be re-used
 *  after this call; see opj_j2k_encode where if l_reuse_data is false it will set l_tilec->data = l_img_comp->data.
 */
extern ArrayData compress_j2k (std::shared_ptr<const OpenJPEGImage>, int bandwith, int frames_per_second, bool threed, bool fourk, std::string comment = "libdcp");


}
