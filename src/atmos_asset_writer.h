/*
    Copyright (C) 2016-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/atmos_asset_writer.h
 *  @brief AtmosAssetWriter class
 */


#ifndef LIBDCP_ATMOS_ASSET_WRITER_H
#define LIBDCP_ATMOS_ASSET_WRITER_H


#include "asset_writer.h"
#include "types.h"
#include "atmos_frame.h"
#include <memory>
#include <boost/filesystem.hpp>


namespace dcp {


class AtmosAsset;


/** @class AtmosAssetWriter
 *  @brief A helper class for writing to AtmosAssets.
 *
 *  Objects of this class can only be created with AtmosAsset::start_write().
 */
class AtmosAssetWriter : public AssetWriter
{
public:
	void write (std::shared_ptr<const AtmosFrame> frame);
	void write (uint8_t const * data, int size);
	bool finalize () override;

private:
	friend class AtmosAsset;

	AtmosAssetWriter (AtmosAsset *, boost::filesystem::path);

	/* do this with an opaque pointer so we don't have to include
	   ASDCP headers
	*/
	struct ASDCPState;
	std::shared_ptr<ASDCPState> _state;

	AtmosAsset* _asset;
};


}


#endif
