/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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

/** @file  src/kdm.cc
 *  @brief KDM and KDMKey classes.
 */

#include "util.h"
#include "kdm.h"
#include "compose.hpp"
#include "exceptions.h"
#include "signer.h"
#include "cpl.h"
#include "mxf.h"
#include "kdm_smpte_xml.h"
#include "AS_DCP.h"
#include "KM_util.h"
#include <libcxml/cxml.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <boost/algorithm/string.hpp>
#include <iomanip>
#include <algorithm>

using std::list;
using std::string;
using std::stringstream;
using std::map;
using std::hex;
using std::setw;
using std::setfill;
using std::cout;
using boost::shared_ptr;
using namespace dcp;

KDM::KDM (boost::filesystem::path kdm, boost::filesystem::path private_key)
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

	/* Read the encrypted keys from the XML */
	/* XXX: this should be reading more stuff from the XML to fill our member variables */

	list<string> encrypted_keys;
	cxml::Document doc ("DCinemaSecurityMessage");
	doc.read_file (kdm.string ());

	shared_ptr<cxml::Node> authenticated_private = doc.node_child ("AuthenticatedPrivate");
	list<shared_ptr<cxml::Node> > encrypted_key_tags = authenticated_private->node_children ("EncryptedKey");
	for (list<shared_ptr<cxml::Node> >::const_iterator i = encrypted_key_tags.begin(); i != encrypted_key_tags.end(); ++i) {
		encrypted_keys.push_back ((*i)->node_child("CipherData")->string_child ("CipherValue"));
	}
	
	/* Use the private key to decrypt the keys */

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

KDM::KDM (
	boost::shared_ptr<const CPL> cpl,
	boost::shared_ptr<const Signer> signer,
	boost::shared_ptr<const Certificate> recipient_cert,
	boost::posix_time::ptime not_valid_before, boost::posix_time::ptime not_valid_after,
	string annotation_text, string issue_date
	)
	: _id (make_uuid ())
	, _annotation_text (annotation_text)
	, _issue_date (issue_date)
	, _recipient_cert (recipient_cert)
	, _cpl (cpl)
	, _signer (signer)
	, _not_valid_before (not_valid_before)
	, _not_valid_after (not_valid_after)
	, _device_list_identifier_id (make_uuid ())
{
	/* Set up our KDMKey objects.  This extracts Key objects from each MXF asset and
	   puts them (with other stuff) into KDMKey objects.
	*/
	list<shared_ptr<const Content> > content = cpl->content ();
	for (list<shared_ptr<const Content> >::iterator i = content.begin(); i != content.end(); ++i) {
		/* XXX: non-MXF assets? */
		shared_ptr<const MXF> mxf = boost::dynamic_pointer_cast<const MXF> (*i);
		if (mxf) {
			_keys.push_back (
				KDMKey (
					signer, cpl->id (), mxf->key_type (), mxf->key_id (),
					not_valid_before, not_valid_after, mxf->key().get()
					)
				);
		}
	}
}

void
KDM::authenticated_public (xmlpp::Element* node, map<string, xmlpp::Attribute *>& references) const
{
	references["ID_AuthenticatedPublic"] = node->set_attribute ("Id", "ID_AuthenticatedPublic");
	node->add_child("MessageId")->add_child_text ("urn:uuid:" + _id);
	node->add_child("MessageType")->add_child_text ("http://www.smpte-ra.org/430-1/2006/KDM#kdm-key-type");
	node->add_child("AnnotationText")->add_child_text (_annotation_text);
	node->add_child("IssueDate")->add_child_text (_issue_date);

	/* Signer */
	xmlpp::Element* signer = node->add_child ("Signer");
	signer->add_child("X509IssuerName", "ds")->add_child_text (_signer->certificates().leaf()->issuer ());
	signer->add_child("X509SerialNumber", "ds")->add_child_text (_signer->certificates().leaf()->serial ());

	/* Everything else is in RequiredExtensions/KDMRequiredExtensions */
	xmlpp::Element* kdm_required_extensions = node->add_child("RequiredExtensions")->add_child("KDMRequiredExtensions");
	kdm_required_extensions->set_attribute ("xmlns", "http://www.smpte-ra.org/schemas/430-1/2006/KDM");

	/* Recipient */
	xmlpp::Element* recipient = kdm_required_extensions->add_child ("Recipient");
	xmlpp::Element* x509_issuer_serial = recipient->add_child ("X509IssuerSerial");
	x509_issuer_serial->add_child("X509IssuerName", "ds")->add_child_text (_recipient_cert->issuer ());
	x509_issuer_serial->add_child("X509SerialNumber", "ds")->add_child_text (_recipient_cert->serial ());
	recipient->add_child("X509SubjectName")->add_child_text (_recipient_cert->subject ());

	kdm_required_extensions->add_child("CompositionPlaylistId")->add_child_text ("urn:uuid:" + _cpl->id ());
	/* XXX: no ContentAuthenticator */
	kdm_required_extensions->add_child("ContentTitleText")->add_child_text (_cpl->content_title_text ());
	kdm_required_extensions->add_child("ContentKeysNotValidBefore")->add_child_text (ptime_to_string (_not_valid_before));
	kdm_required_extensions->add_child("ContentKeysNotValidAfter")->add_child_text (ptime_to_string (_not_valid_after));

	/* AuthorizedDeviceInfo */
	xmlpp::Element* authorized_device_info = kdm_required_extensions->add_child("AuthorizedDeviceInfo");
	authorized_device_info->add_child ("DeviceListIdentifier")->add_child_text ("urn:uuid:" + _device_list_identifier_id);
	string n = _recipient_cert->common_name ();
	if (n.find (".") != string::npos) {
		n = n.substr (n.find (".") + 1);
	}
	authorized_device_info->add_child ("DeviceListDescription")->add_child_text (n);
	xmlpp::Element* device_list = authorized_device_info->add_child ("DeviceList");
	/* Sometimes digital_cinema_tools uses this magic thumbprint instead of that from an actual
	   recipient certificate.  KDMs delivered to City Screen appear to use the same thing.
	*/
	device_list->add_child("CertificateThumbprint")->add_child_text ("2jmj7l5rSw0yVb/vlWAYkK/YBwk=");
	
	/* KeyIdList */
	xmlpp::Element* key_id_list = kdm_required_extensions->add_child("KeyIdList");
	list<shared_ptr<const Content> > content = _cpl->content ();
	for (list<shared_ptr<const Content> >::iterator i = content.begin(); i != content.end(); ++i) {
		/* XXX: non-MXF assets? */
		shared_ptr<const MXF> mxf = boost::dynamic_pointer_cast<const MXF> (*i);
		if (mxf) {
			xmlpp::Element* typed_key_id = key_id_list->add_child ("TypedKeyId");
			typed_key_id->add_child("KeyType")->add_child_text (mxf->key_type ());
			typed_key_id->add_child("KeyId")->add_child_text ("urn:uuid:" + mxf->key_id ());
		}
	}
		
	/* ForensicMarkFlagList */
	xmlpp::Element* forensic_mark_flag_list = kdm_required_extensions->add_child ("ForensicMarkFlagList");
	forensic_mark_flag_list->add_child("ForensicMarkFlag")->add_child_text ("http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-picture-disable");
	forensic_mark_flag_list->add_child("ForensicMarkFlag")->add_child_text ("http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-audio-disable");
		
	node->add_child ("NonCriticalExtensions");
}

void
KDM::authenticated_private (xmlpp::Element* node, map<string, xmlpp::Attribute *>& references) const
{
	references["ID_AuthenticatedPrivate"] = node->set_attribute ("Id", "ID_AuthenticatedPrivate");

	for (list<KDMKey>::const_iterator i = _keys.begin(); i != _keys.end(); ++i) {
		xmlpp::Element* encrypted_key = node->add_child ("EncryptedKey", "enc");
		xmlpp::Element* encryption_method = encrypted_key->add_child ("EncryptionMethod", "enc");
		encryption_method->set_attribute ("Algorithm", "http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p");
		xmlpp::Element* digest_method = encryption_method->add_child ("DigestMethod", "ds");
		digest_method->set_attribute ("Algorithm", "http://www.w3.org/2000/09/xmldsig#sha1");
		xmlpp::Element* cipher_data = encrypted_key->add_child ("CipherData", "enc");
		cipher_data->add_child("CipherValue", "enc")->add_child_text (i->encrypted_base64 (_recipient_cert));
	}
}

void
KDM::signature (xmlpp::Element* node, map<string, xmlpp::Attribute *> const & references) const
{
	xmlpp::Element* signed_info = node->add_child ("SignedInfo", "ds");
	signed_info->add_child ("CanonicalizationMethod", "ds")->set_attribute ("Algorithm", "http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments");
	signed_info->add_child ("SignatureMethod", "ds")->set_attribute ("Algorithm", "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256");

	for (map<string, xmlpp::Attribute *>::const_iterator i = references.begin(); i != references.end(); ++i) {
		xmlpp::Element* reference = signed_info->add_child ("Reference", "ds");
		reference->set_attribute ("URI", "#" + i->first);
		reference->add_child("DigestMethod", "ds")->set_attribute ("Algorithm", "http://www.w3.org/2001/04/xmlenc#sha256");
		reference->add_child("DigestValue", "ds")->add_child_text ("");
	}
	
	node->add_child("SignatureValue", "ds")->add_child_text ("");
	node->add_child("KeyInfo", "ds");
}

void
KDM::as_xml (boost::filesystem::path path) const
{
	FILE* f = fopen_boost (path, "w");
	string const x = as_xml ();
	fwrite (x.c_str(), 1, x.length(), f);
	fclose (f);
}
       
string
KDM::as_xml () const
{
	xmlpp::Document document;
	xmlpp::Element* root = document.create_root_node ("DCinemaSecurityMessage", "http://www.smpte-ra.org/schemas/430-3/2006/ETM");
	root->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "ds");
	root->set_namespace_declaration ("http://www.w3.org/2001/04/xmlenc#", "enc");

	map<string, xmlpp::Attribute *> references;
	authenticated_public (root->add_child ("AuthenticatedPublic"), references);
	authenticated_private (root->add_child ("AuthenticatedPrivate"), references);

	xmlpp::Element* signature_node = root->add_child ("Signature", "ds");
	signature (signature_node, references);

	for (map<string, xmlpp::Attribute*>::const_iterator i = references.begin(); i != references.end(); ++i) {
		xmlAddID (0, document.cobj(), (const xmlChar *) i->first.c_str(), i->second->cobj ());
	}

	_signer->add_signature_value (signature_node, "ds");
	
	/* This must *not* be the _formatted version, otherwise the signature
	   will be wrong.
	*/
	return document.write_to_string ("UTF-8");
}

KDMKey::KDMKey (
	boost::shared_ptr<const Signer> signer,
	string cpl_id,
	string key_type,
	string key_id,
	boost::posix_time::ptime from,
	boost::posix_time::ptime until,
	Key key
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
KDMKey::encrypted_base64 (boost::shared_ptr<const Certificate> recipient_cert) const
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
dcp::operator== (dcp::KDMKey const & a, dcp::KDMKey const & b)
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
