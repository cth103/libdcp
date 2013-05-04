/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_MXF_ASSET_H
#define LIBDCP_MXF_ASSET_H

#include <boost/signals2.hpp>
#include "asset.h"

namespace libdcp
{

class MXFMetadata;	

/** @brief Parent class for assets which have MXF files */	
class MXFAsset : public Asset
{
public:
	/** Construct an MXFAsset.
	 *  This class will not write anything to disk in this constructor, but subclasses may.
	 *
	 *  @param directory Directory where MXF file is.
	 *  @param file_name Name of MXF file.
	 */
	MXFAsset (std::string directory, std::string file_name);
	
	/** Construct an MXFAsset.
	 *  This class will not write anything to disk in this constructor, but subclasses may.
	 *
	 *  @param directory Directory where MXF file is.
	 *  @param file_name Name of MXF file.
	 *  @param progress Signal to use to inform of progress, or 0.
	 *  @param edit_rate Edit rate in frames per second (usually equal to the video frame rate).
	 *  @param intrinsic_duration Duration of the whole asset in frames.
	 */
	MXFAsset (std::string directory, std::string file_name, boost::signals2::signal<void (float)>* progress, int edit_rate, int intrinsic_duration);

	virtual bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;

	/** Fill in a ADSCP::WriteInfo struct.
	 *  @param w struct to fill in.
	 *  @param uuid uuid to use.
	 */
	static void fill_writer_info (ASDCP::WriterInfo* w, std::string uuid, MXFMetadata const & metadata);

protected:
	
	/** Signal to emit to report progress, or 0 */
	boost::signals2::signal<void (float)>* _progress;
};

}

#endif
