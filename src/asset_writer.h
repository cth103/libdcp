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


/** @file  src/asset_writer.h
 *  @brief AssetWriter class
 */


#ifndef LIBDCP_ASSET_WRITER_H
#define LIBDCP_ASSET_WRITER_H


#include "types.h"
#include "crypto_context.h"
#include <boost/filesystem.hpp>


namespace dcp {


class MXF;


/** @class AssetWriter
 *  @brief Parent class for classes which can write MXF-based assets.
 *
 *  The AssetWriter lasts for the duration of the write and is then discarded.
 *  They can only be created by calling start_write() on an appropriate Asset object.
 */
class AssetWriter
{
public:
	AssetWriter (AssetWriter const&) = delete;
	AssetWriter& operator= (AssetWriter const&) = delete;

	virtual ~AssetWriter () {}

	/** @return true if anything was written by this writer */
	virtual bool finalize ();

	int64_t frames_written () const {
		return _frames_written;
	}

protected:
	AssetWriter (MXF* mxf, boost::filesystem::path file);

	/** MXF that we are writing */
	MXF* _mxf = nullptr;
	/** File that we are writing to */
	boost::filesystem::path _file;
	/** Number of `frames' written so far; the definition of a frame
	 *  varies depending on the subclass.
	 */
	int64_t _frames_written = 0;
	/** true if finalize() has been called on this object */
	bool _finalized = false;
	/** true if something has been written to this asset */
	bool _started = false;
	std::shared_ptr<EncryptionContext> _crypto_context;
};

}

#endif
