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

#include "asset.h"

namespace libdcp
{

class MXFAsset : public Asset
{
public:
	/** Construct an MXFAsset.
	 *  @param directory Directory where MXF file is.
	 *  @param file_name Name of MXF file.
	 *  @param progress Signal to inform of progress.
	 *  @param fps Frames per second.
	 *  @param length Length in frames.
	 */
	MXFAsset (std::string directory, std::string file_name, sigc::signal1<void, float>* progress, int fps, int entry_point, int length);

	virtual std::list<std::string> equals (boost::shared_ptr<const Asset> other, EqualityOptions opt) const;
	
	int length () const;

protected:
	/** Fill in a ADSCP::WriteInfo struct.
	 *  @param w struct to fill in.
	 */
	void fill_writer_info (ASDCP::WriterInfo* w) const;

	/** Signal to emit to report progress */
	sigc::signal1<void, float>* _progress;
	/** Frames per second */
	int _fps;
	int _entry_point;
	/** Length in frames */
	int _length;
};

}

#endif
