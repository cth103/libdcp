/*
    Copyright (C) 2016 Carl Hetherington <cth@carlh.net>

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
*/

#ifndef LIBDCP_ASSET_READER_H
#define LIBDCP_ASSET_READER_H

namespace ASDCP {
	class AESDecContext;
}

namespace dcp {

class MXF;

class AssetReader
{
public:
	AssetReader (MXF const * mxf);
	virtual ~AssetReader ();

protected:
	ASDCP::AESDecContext* _decryption_context;
};

}

#endif
