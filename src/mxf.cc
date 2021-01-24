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


/** @file  src/mxf.cc
 *  @brief MXF class
 */


#include "raw_convert.h"
#include "mxf.h"
#include "util.h"
#include "metadata.h"
#include "exceptions.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_prng.h>
#include <asdcp/KM_util.h>
#include <libxml++/nodes/element.h>
#include <boost/filesystem.hpp>
#include <iostream>


using std::string;
using std::cout;
using std::list;
using std::pair;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using namespace dcp;


MXF::MXF ()
	: _context_id (make_uuid ())
{
	/* Subclasses can create MXFs with unspecified _standard but are expected to fill
	   _standard in once the MXF is read.
	*/
}


MXF::MXF (Standard standard)
	: _context_id (make_uuid ())
	, _standard (standard)
{

}


void
MXF::fill_writer_info (ASDCP::WriterInfo* writer_info, string id) const
{
	writer_info->ProductVersion = _metadata.product_version;
	writer_info->CompanyName = _metadata.company_name;
	writer_info->ProductName = _metadata.product_name;

	DCP_ASSERT (_standard);
	if (_standard == Standard::INTEROP) {
		writer_info->LabelSetType = ASDCP::LS_MXF_INTEROP;
	} else {
		writer_info->LabelSetType = ASDCP::LS_MXF_SMPTE;
	}
	unsigned int c;
	Kumu::hex2bin (id.c_str(), writer_info->AssetUUID, Kumu::UUID_Length, &c);
	DCP_ASSERT (c == Kumu::UUID_Length);

	writer_info->UsesHMAC = true;

	if (_key_id) {
		Kumu::hex2bin (_context_id.c_str(), writer_info->ContextID, Kumu::UUID_Length, &c);
		writer_info->EncryptedEssence = true;

		unsigned int c;
		Kumu::hex2bin (_key_id.get().c_str(), writer_info->CryptographicKeyID, Kumu::UUID_Length, &c);
		DCP_ASSERT (c == Kumu::UUID_Length);
	}
}


void
MXF::set_key (Key key)
{
	_key = key;

	if (!_key_id) {
		/* No key ID so far; we now need one */
		_key_id = make_uuid ();
	}
}


string
MXF::read_writer_info (ASDCP::WriterInfo const & info)
{
	char buffer[64];

	if (info.EncryptedEssence) {
		Kumu::bin2UUIDhex (info.CryptographicKeyID, ASDCP::UUIDlen, buffer, sizeof (buffer));
		_key_id = buffer;
	}

	switch (info.LabelSetType) {
	case ASDCP::LS_MXF_INTEROP:
		_standard = Standard::INTEROP;
		break;
	case ASDCP::LS_MXF_SMPTE:
		_standard = Standard::SMPTE;
		break;
	default:
		throw ReadError ("Unrecognised label set type in MXF");
	}

	_metadata.read (info);

	Kumu::bin2UUIDhex (info.AssetUUID, ASDCP::UUIDlen, buffer, sizeof (buffer));
	return buffer;
}
