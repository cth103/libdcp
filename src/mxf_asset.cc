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

/** @file  src/asset.cc
 *  @brief Parent class for assets of DCPs made up of MXF files.
 */

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <libxml++/nodes/element.h>
#include "AS_DCP.h"
#include "KM_prng.h"
#include "KM_util.h"
#include "mxf_asset.h"
#include "util.h"
#include "metadata.h"
#include "exceptions.h"

using std::string;
using std::list;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using namespace libdcp;

MXFAsset::MXFAsset (string directory, string file_name, boost::signals2::signal<void (float)>* progress, int fps, int entry_point, int length, bool encrypted)
	: Asset (directory, file_name)
	, _progress (progress)
	, _fps (fps)
	, _entry_point (entry_point)
	, _length (length)
	, _encrypted (encrypted)
	, _encryption_context (0)
{
	if (_encrypted) {
		_key_id = make_uuid ();
		uint8_t key_buffer[ASDCP::KeyLen];
		Kumu::FortunaRNG rng;
		rng.FillRandom (key_buffer, ASDCP::KeyLen);
		char key_string[ASDCP::KeyLen * 4];
		Kumu::bin2hex (key_buffer, ASDCP::KeyLen, key_string, ASDCP::KeyLen * 4);
		_key_value = key_string;
			
		_encryption_context = new ASDCP::AESEncContext;
		if (ASDCP_FAILURE (_encryption_context->InitKey (key_buffer))) {
			throw MiscError ("could not set up encryption context");
		}

		uint8_t cbc_buffer[ASDCP::CBC_BLOCK_SIZE];
		
		if (ASDCP_FAILURE (_encryption_context->SetIVec (rng.FillRandom (cbc_buffer, ASDCP::CBC_BLOCK_SIZE)))) {
			throw MiscError ("could not set up CBC initialization vector");
		}
	}
}

MXFAsset::~MXFAsset ()
{
	delete _encryption_context;
}

void
MXFAsset::fill_writer_info (ASDCP::WriterInfo* writer_info) const
{
	writer_info->ProductVersion = Metadata::instance()->product_version;
	writer_info->CompanyName = Metadata::instance()->company_name;
	writer_info->ProductName = Metadata::instance()->product_name.c_str();

	writer_info->LabelSetType = ASDCP::LS_MXF_SMPTE;
	unsigned int c;
	Kumu::hex2bin (_uuid.c_str(), writer_info->AssetUUID, Kumu::UUID_Length, &c);
	assert (c == Kumu::UUID_Length);

	if (_encrypted) {
		Kumu::GenRandomUUID (writer_info->ContextID);
		writer_info->EncryptedEssence = true;

		unsigned int c;
		Kumu::hex2bin (_key_id.c_str(), writer_info->CryptographicKeyID, Kumu::UUID_Length, &c);
		assert (c == Kumu::UUID_Length);
	}
}

bool
MXFAsset::equals (shared_ptr<const Asset> other, EqualityOptions, list<string>& notes) const
{
	shared_ptr<const MXFAsset> other_mxf = dynamic_pointer_cast<const MXFAsset> (other);
	if (!other_mxf) {
		notes.push_back ("comparing an MXF asset with a non-MXF asset");
		return false;
	}
	
	if (_file_name != other_mxf->_file_name) {
		notes.push_back ("MXF names differ");
		return false;
	}

	if (_fps != other_mxf->_fps) {
		notes.push_back ("MXF frames per second differ");
		return false;
	}
	
	if (_length != other_mxf->_length) {
		notes.push_back ("MXF lengths differ");
		return false;
	}

	return true;
}

int
MXFAsset::length () const
{
	return _length;
}

void
MXFAsset::add_typed_key_id (xmlpp::Element* parent) const
{
	xmlpp::Element* typed_key_id = parent->add_child("TypedKeyId");
	typed_key_id->add_child("KeyType")->add_child_text(key_type ());
	typed_key_id->add_child("KeyId")->add_child_text("urn:uuid:" + _key_id);
}
