/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

/** @file  src/asset.cc
 *  @brief Parent class for assets of DCPs made up of MXF files.
 */

#include "raw_convert.h"
#include "AS_DCP.h"
#include "KM_prng.h"
#include "KM_util.h"
#include "mxf.h"
#include "util.h"
#include "metadata.h"
#include "exceptions.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include <libxml++/nodes/element.h>
#include <boost/filesystem.hpp>
#include <iostream>

using std::string;
using std::list;
using std::pair;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using namespace dcp;

MXF::MXF ()
	: _decryption_context (0)
{

}

MXF::~MXF ()
{
	delete _decryption_context;
}

void
MXF::fill_writer_info (ASDCP::WriterInfo* writer_info, string id, Standard standard)
{
	writer_info->ProductVersion = _metadata.product_version;
	writer_info->CompanyName = _metadata.company_name;
	writer_info->ProductName = _metadata.product_name.c_str();

	if (standard == INTEROP) {
		writer_info->LabelSetType = ASDCP::LS_MXF_INTEROP;
	} else {
		writer_info->LabelSetType = ASDCP::LS_MXF_SMPTE;
	}
	unsigned int c;
	Kumu::hex2bin (id.c_str(), writer_info->AssetUUID, Kumu::UUID_Length, &c);
	DCP_ASSERT (c == Kumu::UUID_Length);

	if (_key_id) {
		Kumu::GenRandomUUID (writer_info->ContextID);
		writer_info->EncryptedEssence = true;

		unsigned int c;
		Kumu::hex2bin (_key_id.get().c_str(), writer_info->CryptographicKeyID, Kumu::UUID_Length, &c);
		DCP_ASSERT (c == Kumu::UUID_Length);
	}
}

/** Set the (private) key that will be used to encrypt or decrypt this MXF's content.
 *  This is the top-secret key that is distributed (itself encrypted) to cinemas
 *  via Key Delivery Messages (KDMs).
 *  @param key Key to use.
 */
void
MXF::set_key (Key key)
{
	_key = key;

	if (!_key_id) {
		/* No key ID so far; we now need one */
		_key_id = make_uuid ();
	}
	
	_decryption_context = new ASDCP::AESDecContext;
	if (ASDCP_FAILURE (_decryption_context->InitKey (_key->value ()))) {
		throw MiscError ("could not set up decryption context");
	}
}

string
MXF::read_writer_info (ASDCP::WriterInfo const & info)
{
	char buffer[64];

	Kumu::bin2UUIDhex (info.CryptographicKeyID, ASDCP::UUIDlen, buffer, sizeof (buffer));
	_key_id = buffer;

	_metadata.read (info);
	
	Kumu::bin2UUIDhex (info.AssetUUID, ASDCP::UUIDlen, buffer, sizeof (buffer));
	return buffer;
}
