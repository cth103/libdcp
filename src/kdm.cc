/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

#include <iomanip>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <libcxml/cxml.h>
#include "AS_DCP.h"
#include "KM_util.h"
#include "util.h"
#include "kdm.h"
#include "compose.hpp"
#include "exceptions.h"
#include "signer.h"
#include "cpl.h"
#include "mxf_asset.h"
#include "xml/kdm_smpte.h"
#include "parse/cpl.h"

using std::list;
using std::string;
using std::stringstream;
using std::hex;
using std::setw;
using std::setfill;
using std::cout;
using boost::shared_ptr;
using namespace libdcp;

KDM::KDM (boost::filesystem::path kdm, boost::filesystem::path private_key)
	: _xml_kdm (new xml::DCinemaSecurityMessage (kdm))
{
	/* Read the private key */
	   
	FILE* private_key_file = fopen_boost (private_key, "r");
	if (!private_key_file) {
		throw FileError ("could not find RSA private key file", private_key, errno);
	}
	
	RSA* rsa = PEM_read_RSAPrivateKey (private_key_file, 0, 0, 0);
	fclose (private_key_file);	
	if (!rsa) {
		throw FileError ("could not read RSA private key file", private_key, errno);
	}

	/* Use it to decrypt the keys */

	list<string> encrypted_keys = _xml_kdm->authenticated_private.encrypted_keys;

	for (list<string>::iterator i = encrypted_keys.begin(); i != encrypted_keys.end(); ++i) {

		/* Decode the base-64-encoded cipher value from the KDM */
		unsigned char cipher_value[256];
		int const cipher_value_len = base64_decode (*i, cipher_value, sizeof (cipher_value));

		/* Decrypt it */
		unsigned char* decrypted = new unsigned char[RSA_size(rsa)];
		int const decrypted_len = RSA_private_decrypt (cipher_value_len, cipher_value, decrypted, rsa, RSA_PKCS1_OAEP_PADDING);
		if (decrypted_len == -1) {
			delete[] decrypted;
			throw MiscError (String::compose ("Could not decrypt KDM (%1)", ERR_error_string (ERR_get_error(), 0)));
		}

		_keys.push_back (KDMKey (decrypted, decrypted_len));
		delete[] decrypted;
	}

	RSA_free (rsa);
}

/** @param not_valid_before KDM not-valid-before time in local time.
 *  @param not_valid_after KDM not-valid-after time in local time.
 */
KDM::KDM (
	boost::filesystem::path cpl_file,
	shared_ptr<const Signer> signer,
	shared_ptr<const Certificate> recipient_cert, Key key,
	boost::posix_time::ptime not_valid_before, boost::posix_time::ptime not_valid_after,
	string annotation_text, string issue_date, KDM::Formulation formulation
	)
	: _xml_kdm (new xml::DCinemaSecurityMessage)
{
	/* This is all a bit of a hack, and should hopefully be nicer in libdcp1.
	   We load in the CPL file using our parser here, and extract everything
	   we need.  This is much better than needing the whole DCP and going through
	   the dance of setting the MXF's keys and so on.
	*/

	parse::CPL cpl (cpl_file);
	
	xml::AuthenticatedPublic& apu = _xml_kdm->authenticated_public;

	/* AuthenticatedPublic */

	apu.message_id = "urn:uuid:" + make_uuid ();
	apu.message_type = "http://www.smpte-ra.org/430-1/2006/KDM#kdm-key-type";
	apu.annotation_text = annotation_text;
	apu.issue_date = issue_date;
	apu.signer.x509_issuer_name = signer->certificates().leaf()->issuer ();
	apu.signer.x509_serial_number = signer->certificates().leaf()->serial ();
	apu.recipient.x509_issuer_serial.x509_issuer_name = recipient_cert->issuer ();
	apu.recipient.x509_issuer_serial.x509_serial_number = recipient_cert->serial ();
	apu.recipient.x509_subject_name = recipient_cert->subject ();
	apu.composition_playlist_id = cpl.id;
	if (formulation == DCI_ANY || formulation == DCI_SPECIFIC) {
		apu.content_authenticator = signer->certificates().leaf()->thumbprint ();
	}
	apu.content_title_text = cpl.annotation_text;
	apu.content_keys_not_valid_before = ptime_to_string (not_valid_before);
	apu.content_keys_not_valid_after = ptime_to_string (not_valid_after);
	apu.authorized_device_info.device_list_identifier = "urn:uuid:" + make_uuid ();
	string n = recipient_cert->common_name ();
	if (n.find (".") != string::npos) {
		n = n.substr (n.find (".") + 1);
	}
	apu.authorized_device_info.device_list_description = n;

	if (formulation == MODIFIED_TRANSITIONAL_1 || formulation == DCI_ANY) {
		/* Use the "assume trust" thumbprint */
		apu.authorized_device_info.device_list.push_back ("2jmj7l5rSw0yVb/vlWAYkK/YBwk=");
	} else if (formulation == DCI_SPECIFIC) {
		/* Use the recipient thumbprint */
		apu.authorized_device_info.device_list.push_back (recipient_cert->thumbprint ());
	}

	for (list<shared_ptr<parse::Reel> >::const_iterator i = cpl.reels.begin(); i != cpl.reels.end(); ++i) {
		/* XXX: subtitle assets? */
		if ((*i)->asset_list->main_picture) {
			apu.key_id_list.push_back (xml::TypedKeyId ("MDIK", (*i)->asset_list->main_picture->key_id));
		}
		if ((*i)->asset_list->main_stereoscopic_picture) {
			apu.key_id_list.push_back (xml::TypedKeyId ("MDIK", (*i)->asset_list->main_stereoscopic_picture->key_id));
		}
		if ((*i)->asset_list->main_sound) {
			apu.key_id_list.push_back (xml::TypedKeyId ("MDAK", (*i)->asset_list->main_sound->key_id));
		}
	}

	apu.forensic_mark_flag_list.push_back ("http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-picture-disable");
	apu.forensic_mark_flag_list.push_back ("http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-audio-disable");

	/* AuthenticatedPrivate */

	for (list<shared_ptr<parse::Reel> >::iterator i = cpl.reels.begin(); i != cpl.reels.end(); ++i) {
		/* XXX: subtitle assets? */

		if ((*i)->asset_list->main_picture) {
			if ((*i)->asset_list->main_picture->key_id.empty ()) {
				throw NotEncryptedError ("MainPicture");
			}
			
			KDMKey kkey (
				signer, cpl.id.substr (9), "MDIK", (*i)->asset_list->main_picture->key_id.substr (9),
				not_valid_before, not_valid_after, key
				);

			_keys.push_back (kkey);
			_xml_kdm->authenticated_private.encrypted_keys.push_back (kkey.encrypted_base64 (recipient_cert));
		}

		if ((*i)->asset_list->main_stereoscopic_picture) {
			if ((*i)->asset_list->main_stereoscopic_picture->key_id.empty ()) {
				throw NotEncryptedError ("MainStereoscopicPicture");
			}

			KDMKey kkey (
				signer, cpl.id.substr (9), "MDIK", (*i)->asset_list->main_stereoscopic_picture->key_id.substr (9),
				not_valid_before, not_valid_after, key
				);

			_keys.push_back (kkey);
			_xml_kdm->authenticated_private.encrypted_keys.push_back (kkey.encrypted_base64 (recipient_cert));
		}
		
		if ((*i)->asset_list->main_sound) {
			if ((*i)->asset_list->main_sound->key_id.empty ()) {
				throw NotEncryptedError ("MainSound");
			}
			
			KDMKey kkey (
				signer, cpl.id.substr (9), "MDAK", (*i)->asset_list->main_sound->key_id.substr (9),
				not_valid_before, not_valid_after, key
				);
			
			_keys.push_back (kkey);
			_xml_kdm->authenticated_private.encrypted_keys.push_back (kkey.encrypted_base64 (recipient_cert));
		}
	}

	/* Signature */

	shared_ptr<xmlpp::Document> doc = _xml_kdm->as_xml ();
	shared_ptr<cxml::Node> root (new cxml::Node (doc->get_root_node ()));
	xmlpp::Node* signature = root->node_child("Signature")->node();
	signer->add_signature_value (signature, "ds");
	_xml_kdm->signature = xml::Signature (shared_ptr<cxml::Node> (new cxml::Node (signature)));
}

KDM::KDM (KDM const & other)
	: _keys (other._keys)
	, _xml_kdm (new xml::DCinemaSecurityMessage (*other._xml_kdm.get()))
{

}

KDM &
KDM::operator= (KDM const & other)
{
	if (this == &other) {
		return *this;
	}

	_keys = other._keys;
	_xml_kdm.reset (new xml::DCinemaSecurityMessage (*other._xml_kdm.get ()));

	return *this;
}
     
void
KDM::as_xml (boost::filesystem::path path) const
{
	shared_ptr<xmlpp::Document> doc = _xml_kdm->as_xml ();
	/* This must *not* be the _formatted version, otherwise the signature
	   will be wrong.
	*/
	doc->write_to_file (path.string(), "UTF-8");
}

string
KDM::as_xml () const
{
	shared_ptr<xmlpp::Document> doc = _xml_kdm->as_xml ();
	/* This must *not* be the _formatted version, otherwise the signature
	   will be wrong.
	*/
	return doc->write_to_string ("UTF-8");
}

KDMKey::KDMKey (
	shared_ptr<const Signer> signer, string cpl_id, string key_type, string key_id, boost::posix_time::ptime from, boost::posix_time::ptime until, Key key
	)
	: _cpl_id (cpl_id)
	, _key_type (key_type)
	, _key_id (key_id)
	, _not_valid_before (ptime_to_string (from))
	, _not_valid_after (ptime_to_string (until))
	, _key (key)
{
	base64_decode (signer->certificates().leaf()->thumbprint (), _signer_thumbprint, 20);
}

KDMKey::KDMKey (uint8_t const * raw, int len)
{
	switch (len) {
	case 134:
		/* interop */
		/* [0-15] is structure id (fixed sequence specified by standard) */
		raw += 16;
		get (_signer_thumbprint, &raw, 20);
		_cpl_id = get_uuid (&raw);
		_key_id = get_uuid (&raw);
		_not_valid_before = get (&raw, 25);
		_not_valid_after = get (&raw, 25);
		_key = Key (raw);
		break;
	case 138:
		/* SMPTE */
		/* [0-15] is structure id (fixed sequence specified by standard) */
		raw += 16;
		get (_signer_thumbprint, &raw, 20);
		_cpl_id = get_uuid (&raw);
		_key_type = get (&raw, 4);
		_key_id = get_uuid (&raw);
		_not_valid_before = get (&raw, 25);
		_not_valid_after = get (&raw, 25);
		_key = Key (raw);
		break;
	default:
		assert (false);
	}
}

KDMKey::KDMKey (KDMKey const & other)
	: _cpl_id (other._cpl_id)
	, _key_type (other._key_type)
	, _key_id (other._key_id)
	, _not_valid_before (other._not_valid_before)
	, _not_valid_after (other._not_valid_after)
	, _key (other._key)
{
	memcpy (_signer_thumbprint, other._signer_thumbprint, 20);
}

KDMKey &
KDMKey::operator= (KDMKey const & other)
{
	if (&other == this) {
		return *this;
	}

	_cpl_id = other._cpl_id;
	_key_type = other._key_type;
	_key_id = other._key_id;
	_not_valid_before = other._not_valid_before;
	_not_valid_after = other._not_valid_after;
	_key = other._key;
	memcpy (_signer_thumbprint, other._signer_thumbprint, 20);

	return *this;
}

string
KDMKey::encrypted_base64 (shared_ptr<const Certificate> recipient_cert) const
{
	assert (_key_type.length() == 4);
	assert (_not_valid_before.length() == 25);
	assert (_not_valid_after.length() == 25);
	
	/* XXX: SMPTE only */
	uint8_t block[138];
	uint8_t* p = block;

	/* Magic value specified by SMPTE S430-1-2006 */
	uint8_t structure_id[] = { 0xf1, 0xdc, 0x12, 0x44, 0x60, 0x16, 0x9a, 0x0e, 0x85, 0xbc, 0x30, 0x06, 0x42, 0xf8, 0x66, 0xab };
	put (&p, structure_id, 16);
	put (&p, _signer_thumbprint, 20);
	put_uuid (&p, _cpl_id);
	put (&p, _key_type);
	put_uuid (&p, _key_id);
	put (&p, _not_valid_before);
	put (&p, _not_valid_after);
	put (&p, _key.value(), ASDCP::KeyLen);

	/* Encrypt using the projector's public key */
	RSA* rsa = recipient_cert->public_key ();
	unsigned char encrypted[RSA_size(rsa)];
	int const encrypted_len = RSA_public_encrypt (p - block, block, encrypted, rsa, RSA_PKCS1_OAEP_PADDING);
	if (encrypted_len == -1) {
		throw MiscError (String::compose ("Could not encrypt KDM (%1)", ERR_error_string (ERR_get_error(), 0)));
	}

	/* Lazy overallocation */
	char out[encrypted_len * 2];
	Kumu::base64encode (encrypted, encrypted_len, out, encrypted_len * 2);
	int const N = strlen (out);
	stringstream lines;
	for (int i = 0; i < N; ++i) {
		if (i > 0 && (i % 64) == 0) {
			lines << "\n";
		}
		lines << out[i];
	}

	return lines.str ();
}

string
KDMKey::get (uint8_t const ** p, int N) const
{
	string g;
	for (int i = 0; i < N; ++i) {
		g += **p;
		(*p)++;
	}

	return g;
}

void
KDMKey::get (uint8_t* o, uint8_t const ** p, int N) const
{
	memcpy (o, *p, N);
	*p += N;
}

string
KDMKey::get_uuid (unsigned char const ** p) const
{
	stringstream g;
	
	for (int i = 0; i < 16; ++i) {
		g << setw(2) << setfill('0') << hex << static_cast<int> (**p);
		(*p)++;
		if (i == 3 || i == 5 || i == 7 || i == 9) {
			g << '-';
		}
	}

	return g.str ();
}

void
KDMKey::put (uint8_t ** d, uint8_t const * s, int N) const
{
	memcpy (*d, s, N);
	(*d) += N;
}

void
KDMKey::put (uint8_t ** d, string s) const
{
	memcpy (*d, s.c_str(), s.length());
	(*d) += s.length();
}

void
KDMKey::put_uuid (uint8_t ** d, string id) const
{
	id.erase (std::remove (id.begin(), id.end(), '-'));
	for (int i = 0; i < 32; i += 2) {
		stringstream s;
		s << id[i] << id[i + 1];
		int h;
		s >> hex >> h;
		**d = h;
		(*d)++;
	}
}

bool
libdcp::operator== (libdcp::KDMKey const & a, libdcp::KDMKey const & b)
{
	if (memcmp (a._signer_thumbprint, b._signer_thumbprint, 20) != 0) {
		return false;
	}

	return (
		a._cpl_id == b._cpl_id &&
		a._key_type == b._key_type &&
		a._key_id == b._key_id &&
		a._not_valid_before == b._not_valid_before &&
		a._not_valid_after == b._not_valid_after &&
		a._key == b._key
		);
}
