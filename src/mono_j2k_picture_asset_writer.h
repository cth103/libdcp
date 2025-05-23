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


/** @file  src/mono_j2k_picture_asset_writer.h
 *  @brief MonoJ2KPictureAssetWriter class
 */


#ifndef LIBDCP_MONO_J2K_PICTURE_ASSET_WRITER_H
#define LIBDCP_MONO_J2K_PICTURE_ASSET_WRITER_H


#include "j2k_picture_asset_writer.h"
#include <memory>
#include <boost/utility.hpp>
#include <stdint.h>
#include <string>


namespace dcp {


/** @class MonoJ2KPictureAssetWriter
 *  @brief A helper class for writing to MonoJ2KPictureAssets
 *
 *  Objects of this class can only be created with MonoJ2KPictureAsset::start_write().
 *
 *  Frames can be written to the MonoJ2KPictureAsset by calling write() with a JPEG2000 image
 *  (a verbatim .j2c file).  finalize() should be called after the last frame has been written,
 *  but if it is not, it will be called by the destructor (though in that case any error
 *  during finalization will be ignored).
 */
class MonoJ2KPictureAssetWriter : public J2KPictureAssetWriter
{
public:
	~MonoJ2KPictureAssetWriter();

	J2KFrameInfo write(uint8_t const *, int) override;
	void fake_write(J2KFrameInfo const& info) override;
	bool finalize () override;

private:
	friend class MonoJ2KPictureAsset;

	MonoJ2KPictureAssetWriter (J2KPictureAsset* a, boost::filesystem::path file, bool);

	void start (uint8_t const *, int);

	/* do this with an opaque pointer so we don't have to include
	   ASDCP headers
	*/
	struct ASDCPState;
	std::shared_ptr<ASDCPState> _state;
};


}


#endif
