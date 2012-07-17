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

#ifndef LIBDCP_ASSET_H
#define LIBDCP_ASSET_H

#include <string>
#include <sigc++/sigc++.h>

namespace ASDCP {
	class WriterInfo;
}

namespace libdcp
{

/** Parent class for assets (picture / sound collections) */	
class Asset
{
public:
	Asset (std::string, int, int);

	virtual void write_to_cpl (std::ostream &) const = 0;
	void write_to_pkl (std::ostream &) const;
	void write_to_assetmap (std::ostream &) const;

	/** Emitted with a parameter between 0 and 1 to indicate progress in constructing
	 *  this asset.
	 */
	sigc::signal1<void, float> Progress;

protected:
	void fill_writer_info (ASDCP::WriterInfo *) const;

	/** Path to our MXF file */
	std::string _mxf_path;
	/** Frames per second */
	int _fps;
	/** Length in frames */
	int _length;
	/** Our UUID */
	std::string _uuid;
	/** Digest of our MXF */
	std::string _digest;
};

}

#endif
