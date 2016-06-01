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

#include "asset_reader.h"
#include "mxf.h"
#include "exceptions.h"
#include "AS_DCP.h"

using namespace dcp;

AssetReader::AssetReader (MXF const * mxf)
	: _decryption_context (0)
{
	if (mxf->key()) {
		_decryption_context = new ASDCP::AESDecContext;
		if (ASDCP_FAILURE (_decryption_context->InitKey (mxf->key()->value ()))) {
			throw MiscError ("could not set up decryption context");
		}
	}
}

AssetReader::~AssetReader ()
{
	delete _decryption_context;
}
