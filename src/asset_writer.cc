/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

/** @file  src/asset_writer.h
 *  @brief AssetWriter class.
 */

#include "asset_writer.h"
#include "mxf.h"
#include "dcp_assert.h"
#include "AS_DCP.h"
#include "KM_prng.h"

using namespace dcp;

/** Create an AssetWriter.
 *  @param mxf MXF that we are writing.
 *  @param file File to write to.
 */
AssetWriter::AssetWriter (MXF* mxf, boost::filesystem::path file)
	: _mxf (mxf)
	, _file (file)
	, _frames_written (0)
	, _finalized (false)
	, _encryption_context (0)
{
	if (mxf->key ()) {
		_encryption_context = new ASDCP::AESEncContext;
		if (ASDCP_FAILURE (_encryption_context->InitKey (mxf->key()->value ()))) {
			throw MiscError ("could not set up encryption context");
		}

		uint8_t cbc_buffer[ASDCP::CBC_BLOCK_SIZE];

		Kumu::FortunaRNG rng;
		if (ASDCP_FAILURE (_encryption_context->SetIVec (rng.FillRandom (cbc_buffer, ASDCP::CBC_BLOCK_SIZE)))) {
			throw MiscError ("could not set up CBC initialization vector");
		}
	}
}

AssetWriter::~AssetWriter ()
{
	delete _encryption_context;
}

void
AssetWriter::finalize ()
{
	DCP_ASSERT (!_finalized);
	_finalized = true;
}
