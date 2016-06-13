/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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
AssetWriter::AssetWriter (MXF* mxf, boost::filesystem::path file, Standard standard)
	: _mxf (mxf)
	, _file (file)
	, _frames_written (0)
	, _finalized (false)
	, _started (false)
	, _encryption_context (0)
	, _hmac_context (0)
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

		_hmac_context = new ASDCP::HMACContext;

		ASDCP::LabelSet_t type;
		if (standard == INTEROP) {
			type = ASDCP::LS_MXF_INTEROP;
		} else {
			type = ASDCP::LS_MXF_SMPTE;
		}

		if (ASDCP_FAILURE (_hmac_context->InitKey (mxf->key()->value(), type))) {
			throw MiscError ("could not set up HMAC context");
		}
	}
}

AssetWriter::~AssetWriter ()
{
	delete _encryption_context;
	delete _hmac_context;
}

/** @return true if anything was written by this writer */
bool
AssetWriter::finalize ()
{
	DCP_ASSERT (!_finalized);
	_finalized = true;
	return _started;
}
