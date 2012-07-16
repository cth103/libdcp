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

class Asset
{
public:
	Asset (std::string, int, int);

	virtual void write_to_cpl (std::ostream &) const = 0;
	void write_to_pkl (std::ostream &) const;
	void write_to_assetmap (std::ostream &) const;

	sigc::signal1<void, float> Progress;

protected:
	void fill_writer_info (ASDCP::WriterInfo *) const;

	std::string _mxf_path;
	int _fps;
	int _length;
	std::string _uuid;
	std::string _digest;
};

}

#endif
