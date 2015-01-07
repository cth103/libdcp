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

MXF::MXF (Fraction edit_rate)
	: _edit_rate (edit_rate)
	, _intrinsic_duration (0)
	, _encryption_context (0)
	, _decryption_context (0)
{
	/* _intrinsic_duration must be set up up by a subclass */
}

MXF::MXF (boost::filesystem::path file)
	: Content (file)
	, _intrinsic_duration (0)
	, _encryption_context (0)
	, _decryption_context (0)
{
	/* _edit_rate and _intrinsic_duration must be set up up by a subclass */
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
	DCP_ASSERT (c == Kumu::UUID_Length);

	if (_key) {
		Kumu::GenRandomUUID (writer_info->ContextID);
		writer_info->EncryptedEssence = true;

		unsigned int c;
		Kumu::hex2bin (_key_id.c_str(), writer_info->CryptographicKeyID, Kumu::UUID_Length, &c);
		DCP_ASSERT (c == Kumu::UUID_Length);
	}
}

bool
MXF::equals (shared_ptr<const Asset> other, EqualityOptions opt, NoteHandler note) const
{
	if (!Content::equals (other, opt, note)) {
		return false;
	}
	
	shared_ptr<const MXF> other_mxf = dynamic_pointer_cast<const MXF> (other);
	if (!other_mxf) {
		return false;
	}
	
	if (_edit_rate != other_mxf->_edit_rate) {
		note (DCP_ERROR, "MXF: edit rates differ");
	 	return false;
	}
	
	if (_intrinsic_duration != other_mxf->_intrinsic_duration) {
	 	note (DCP_ERROR, String::compose ("MXF: intrinsic durations differ (%1 vs %2)", _intrinsic_duration, other_mxf->_intrinsic_duration));
		return false;
	}

	if (_file.leaf() != other_mxf->file().leaf()) {
		if (!opt.mxf_filenames_can_differ) {
			note (DCP_ERROR, "MXF: filenames differ");
			return false;
		} else {
			note (DCP_NOTE, "MXF: filenames differ");
		}
	}
	
	return true;
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

void
MXF::read_writer_info (ASDCP::WriterInfo const & info)
{
	char buffer[64];
	Kumu::bin2UUIDhex (info.AssetUUID, ASDCP::UUIDlen, buffer, sizeof (buffer));
	_id = buffer;
}

string
MXF::pkl_type (Standard standard) const
{
	switch (standard) {
	case INTEROP:
		return String::compose ("application/x-smpte-mxf;asdcpKind=%1", asdcp_kind ());
	case SMPTE:
		return "application/mxf";
	default:
		DCP_ASSERT (false);
	}
}
