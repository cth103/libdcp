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
#include <boost/lexical_cast.hpp>
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
using boost::lexical_cast;
using boost::dynamic_pointer_cast;
using namespace libdcp;

MXFAsset::MXFAsset (string directory, string file_name)
	: Asset (directory, file_name)
	, _progress (0)
	, _encrypted (false)
	, _encryption_context (0)
{

}

MXFAsset::MXFAsset (string directory, string file_name, boost::signals2::signal<void (float)>* progress, int edit_rate, int intrinsic_duration, bool encrypted)
	: Asset (directory, file_name, edit_rate, intrinsic_duration)
	, _progress (progress)
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
MXFAsset::fill_writer_info (ASDCP::WriterInfo* writer_info, string uuid, MXFMetadata const & metadata)
{
	writer_info->ProductVersion = metadata.product_version;
	writer_info->CompanyName = metadata.company_name;
	writer_info->ProductName = metadata.product_name.c_str();

	writer_info->LabelSetType = ASDCP::LS_MXF_SMPTE;
	unsigned int c;
	Kumu::hex2bin (uuid.c_str(), writer_info->AssetUUID, Kumu::UUID_Length, &c);
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
MXFAsset::equals (shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (!Asset::equals (other, opt, note)) {
		return false;
	}
	
	shared_ptr<const MXFAsset> other_mxf = dynamic_pointer_cast<const MXFAsset> (other);
	if (!other_mxf) {
		note (ERROR, "comparing an MXF asset with a non-MXF asset");
		return false;
	}
	
	if (_file_name != other_mxf->_file_name) {
		note (ERROR, "MXF names differ");
		if (!opt.mxf_names_can_differ) {
			return false;
		}
	}
	
	return true;
}

void
MXFAsset::add_typed_key_id (xmlpp::Element* parent) const
{
	xmlpp::Element* typed_key_id = parent->add_child("TypedKeyId");
	typed_key_id->add_child("KeyType")->add_child_text(key_type ());
	typed_key_id->add_child("KeyId")->add_child_text("urn:uuid:" + _key_id);
}

void
MXFAsset::write_to_cpl (xmlpp::Node* node) const
{
	xmlpp::Node* a = node->add_child (cpl_node_name ());
	a->add_child ("Id")->add_child_text ("urn:uuid:" + _uuid);
	a->add_child ("AnnotationText")->add_child_text (_file_name);
	a->add_child ("EditRate")->add_child_text (lexical_cast<string> (_edit_rate) + " 1");
	a->add_child ("IntrinsicDuration")->add_child_text (lexical_cast<string> (_intrinsic_duration));
	a->add_child ("EntryPoint")->add_child_text (lexical_cast<string> (_entry_point));
	a->add_child ("Duration")->add_child_text (lexical_cast<string> (_duration));
	if (_encrypted) {
		a->add_child("KeyId")->add_child_text ("urn:uuid:" + _key_id);
	}
}
