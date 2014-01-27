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

#include "AS_DCP.h"
#include "KM_prng.h"
#include "KM_util.h"
#include "mxf.h"
#include "util.h"
#include "metadata.h"
#include "exceptions.h"
#include "kdm.h"
#include <libxml++/nodes/element.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

using std::string;
using std::list;
using std::pair;
using boost::shared_ptr;
using boost::lexical_cast;
using boost::dynamic_pointer_cast;
using namespace dcp;

MXF::MXF (Fraction edit_rate)
	: Content (edit_rate)
	, _progress (0)
	, _encryption_context (0)
	, _decryption_context (0)
{

}

MXF::MXF (boost::filesystem::path file)
	: Content (file)
	, _progress (0)
	, _encryption_context (0)
	, _decryption_context (0)
{

}

MXF::~MXF ()
{
	delete _encryption_context;
	delete _decryption_context;
}

void
MXF::fill_writer_info (ASDCP::WriterInfo* writer_info, Standard standard)
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
	Kumu::hex2bin (_id.c_str(), writer_info->AssetUUID, Kumu::UUID_Length, &c);
	assert (c == Kumu::UUID_Length);

	if (_key) {
		Kumu::GenRandomUUID (writer_info->ContextID);
		writer_info->EncryptedEssence = true;

		unsigned int c;
		Kumu::hex2bin (_key_id.c_str(), writer_info->CryptographicKeyID, Kumu::UUID_Length, &c);
		assert (c == Kumu::UUID_Length);
	}
}

bool
MXF::equals (shared_ptr<const Content> other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (!Content::equals (other, opt, note)) {
		return false;
	}
	
	shared_ptr<const MXF> other_mxf = dynamic_pointer_cast<const MXF> (other);
	if (!other_mxf) {
		note (ERROR, "comparing an MXF asset with a non-MXF asset");
		return false;
	}
	
	if (_file != other_mxf->file ()) {
		note (ERROR, "MXF names differ");
		if (!opt.mxf_names_can_differ) {
			return false;
		}
	}
	
	return true;
}

void
MXF::set_key (Key key)
{
	_key = key;

	if (_key_id.empty ()) {
		/* No key ID so far; we now need one */
		_key_id = make_uuid ();
	}
	
	_decryption_context = new ASDCP::AESDecContext;
	if (ASDCP_FAILURE (_decryption_context->InitKey (_key->value ()))) {
		throw MiscError ("could not set up decryption context");
	}

	_encryption_context = new ASDCP::AESEncContext;
	if (ASDCP_FAILURE (_encryption_context->InitKey (_key->value ()))) {
		throw MiscError ("could not set up encryption context");
	}
	
	uint8_t cbc_buffer[ASDCP::CBC_BLOCK_SIZE];
	
	Kumu::FortunaRNG rng;
	if (ASDCP_FAILURE (_encryption_context->SetIVec (rng.FillRandom (cbc_buffer, ASDCP::CBC_BLOCK_SIZE)))) {
		throw MiscError ("could not set up CBC initialization vector");
	}
}
