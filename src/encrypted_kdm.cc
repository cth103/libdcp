/*
    Copyright (C) 2013-2015 Carl Hetherington <cth@carlh.net>

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

#include "encrypted_kdm.h"
#include "util.h"
#include "signer.h"
#include <libcxml/cxml.h>
#include <libxml++/document.h>
#include <libxml++/nodes/element.h>
#include <libxml/parser.h>
#include <boost/date_time/posix_time/posix_time.hpp>

using std::list;
using std::string;
using std::map;
using std::pair;
using boost::shared_ptr;
using namespace dcp;

namespace dcp {

/** Namespace for classes used to hold our data; they are internal to this .cc file */	
namespace data {

class Signer
{
public:
	Signer () {}
	
	Signer (shared_ptr<const cxml::Node> node)
		: x509_issuer_name (node->string_child ("X509IssuerName"))
		, x509_serial_number (node->string_child ("X509SerialNumber"))
	{
		
	}

	void as_xml (xmlpp::Element* node) const
	{
		node->add_child("X509IssuerName", "ds")->add_child_text (x509_issuer_name);
		node->add_child("X509SerialNumber", "ds")->add_child_text (x509_serial_number);
	}
	
	string x509_issuer_name;
	string x509_serial_number;
};

class X509Data
{
public:
	X509Data () {}
	
	X509Data (boost::shared_ptr<const cxml::Node> node)
		: x509_issuer_serial (Signer (node->node_child ("X509IssuerSerial")))
		, x509_certificate (node->string_child ("X509Certificate"))
	{
		node->done ();
	}

	void as_xml (xmlpp::Element* node) const
	{
		x509_issuer_serial.as_xml (node->add_child ("X509IssuerSerial", "ds"));
		node->add_child("X509Certificate", "ds")->add_child_text (x509_certificate);
	}
	
	Signer x509_issuer_serial;
	std::string x509_certificate;
};
	
class Reference
{
public:
	Reference () {}
	
	Reference (string u)
		: uri (u)
	{}

	Reference (shared_ptr<const cxml::Node> node)
		: uri (node->string_attribute ("URI"))
		, digest_value (node->string_child ("DigestValue"))
	{

	}
	
	void as_xml (xmlpp::Element* node) const
	{
		node->set_attribute ("URI", uri);
		node->add_child("DigestMethod", "ds")->set_attribute ("Algorithm", "http://www.w3.org/2001/04/xmlenc#sha256");
		node->add_child("DigestValue", "ds")->add_child_text (digest_value);
	}
	
	string uri;
	string digest_value;
};

class SignedInfo
{
public:
	SignedInfo ()
		: authenticated_public ("#ID_AuthenticatedPublic")
		, authenticated_private ("#ID_AuthenticatedPrivate")
	{}

	SignedInfo (shared_ptr<const cxml::Node> node)
	{
		list<shared_ptr<cxml::Node> > references = node->node_children ("Reference");
		for (list<shared_ptr<cxml::Node> >::const_iterator i = references.begin(); i != references.end(); ++i) {
			if ((*i)->string_attribute ("URI") == "#ID_AuthenticatedPublic") {
				authenticated_public = Reference (*i);
			} else if ((*i)->string_attribute ("URI") == "#ID_AuthenticatedPrivate") {
				authenticated_private = Reference (*i);
			}

			/* XXX: do something if we don't recognise the node */
		}
	}

	void as_xml (xmlpp::Element* node) const
	{
		node->add_child ("CanonicalizationMethod", "ds")->set_attribute (
			"Algorithm", "http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments"
			);

		node->add_child ("SignatureMethod", "ds")->set_attribute (
			"Algorithm", "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256"
			);
		
		authenticated_public.as_xml (node->add_child ("Reference", "ds"));
		authenticated_private.as_xml (node->add_child ("Reference", "ds"));
	}
		
private:
	Reference authenticated_public;
	Reference authenticated_private;
};
	
class Signature
{
public:
	Signature () {}

	Signature (shared_ptr<const cxml::Node> node)
		: signed_info (node->node_child ("SignedInfo"))
		, signature_value (node->string_child ("SignatureValue"))
	{
		list<shared_ptr<cxml::Node> > x509_data_nodes = node->node_child("KeyInfo")->node_children ("X509Data");
		for (list<shared_ptr<cxml::Node> >::const_iterator i = x509_data_nodes.begin(); i != x509_data_nodes.end(); ++i) {
			x509_data.push_back (X509Data (*i));
		}
	}

	void as_xml (xmlpp::Node* node) const
	{
		signed_info.as_xml (node->add_child ("SignedInfo", "ds"));
		node->add_child("SignatureValue", "ds")->add_child_text (signature_value);
		
		xmlpp::Element* key_info_node = node->add_child ("KeyInfo", "ds");
		for (std::list<X509Data>::const_iterator i = x509_data.begin(); i != x509_data.end(); ++i) {
			i->as_xml (key_info_node->add_child ("X509Data", "ds"));
		}
	}

	SignedInfo signed_info;
	string signature_value;
	list<X509Data> x509_data;
};

class AuthenticatedPrivate
{
public:
	AuthenticatedPrivate () {}
	
	AuthenticatedPrivate (shared_ptr<const cxml::Node> node)
	{
		list<shared_ptr<cxml::Node> > encrypted_key_nodes = node->node_children ("EncryptedKey");
		for (list<shared_ptr<cxml::Node> >::const_iterator i = encrypted_key_nodes.begin(); i != encrypted_key_nodes.end(); ++i) {
			encrypted_key.push_back ((*i)->node_child("CipherData")->string_child ("CipherValue"));
		}
	}

	void as_xml (xmlpp::Element* node, map<string, xmlpp::Attribute *>& references) const
	{
		references["ID_AuthenticatedPrivate"] = node->set_attribute ("Id", "ID_AuthenticatedPrivate");

		for (list<string>::const_iterator i = encrypted_key.begin(); i != encrypted_key.end(); ++i) {
			xmlpp::Element* encrypted_key = node->add_child ("EncryptedKey", "enc");
			xmlpp::Element* encryption_method = encrypted_key->add_child ("EncryptionMethod", "enc");
			encryption_method->set_attribute ("Algorithm", "http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p");
			xmlpp::Element* digest_method = encryption_method->add_child ("DigestMethod", "ds");
			digest_method->set_attribute ("Algorithm", "http://www.w3.org/2000/09/xmldsig#sha1");
			xmlpp::Element* cipher_data = encrypted_key->add_child ("CipherData", "enc");
			cipher_data->add_child("CipherValue", "enc")->add_child_text (*i);
		}
	}
	
	list<string> encrypted_key;
};

class TypedKeyId
{
public:
	TypedKeyId () {}
	
	TypedKeyId (shared_ptr<const cxml::Node> node)
		: key_type (node->string_child ("KeyType"))
		, key_id (node->string_child ("KeyId").substr (9))
	{

	}

	TypedKeyId (string type, string id)
		: key_type (type)
		, key_id (id)
	{}

	void as_xml (xmlpp::Element* node) const
	{
		node->add_child("KeyType")->add_child_text (key_type);
		node->add_child("KeyId")->add_child_text ("urn:uuid:" + key_id);
	}

	string key_type;
	string key_id;
};

class KeyIdList
{
public:
	KeyIdList () {}
	
	KeyIdList (shared_ptr<const cxml::Node> node)
	{
		list<shared_ptr<cxml::Node> > typed_key_id_nodes = node->node_children ("TypedKeyId");
		for (list<shared_ptr<cxml::Node> >::const_iterator i = typed_key_id_nodes.begin(); i != typed_key_id_nodes.end(); ++i) {
			typed_key_id.push_back (TypedKeyId (*i));
		}
	}

	void as_xml (xmlpp::Element* node) const
	{
		for (list<TypedKeyId>::const_iterator i = typed_key_id.begin(); i != typed_key_id.end(); ++i) {
			i->as_xml (node->add_child("TypedKeyId"));
		}
	}

	list<TypedKeyId> typed_key_id;
};

class AuthorizedDeviceInfo
{
public:
	AuthorizedDeviceInfo () {}
	
	AuthorizedDeviceInfo (shared_ptr<const cxml::Node> node)
		: device_list_identifier (node->string_child ("DeviceListIdentifier").substr (9))
		, device_list_description (node->string_child ("DeviceListDescription"))
		, certificate_thumbprint (node->node_child("DeviceList")->string_child ("CertificateThumbprint"))
	{

	}

	void as_xml (xmlpp::Element* node) const
	{
		node->add_child ("DeviceListIdentifier")->add_child_text ("urn:uuid:" + device_list_identifier);
		node->add_child ("DeviceListDescription")->add_child_text (device_list_description);
		xmlpp::Element* device_list = node->add_child ("DeviceList");
		device_list->add_child("CertificateThumbprint")->add_child_text (certificate_thumbprint);
	}

	/** DeviceListIdentifier without the urn:uuid: prefix */
	string device_list_identifier;
	string device_list_description;
	string certificate_thumbprint;
};

class X509IssuerSerial
{
public:
	X509IssuerSerial () {}
	
	X509IssuerSerial (shared_ptr<const cxml::Node> node)
		: x509_issuer_name (node->string_child ("X509IssuerName"))
		, x509_serial_number (node->string_child ("X509SerialNumber"))
	{

	}

	void as_xml (xmlpp::Element* node) const
	{
		node->add_child("X509IssuerName", "ds")->add_child_text (x509_issuer_name);
		node->add_child("X509SerialNumber", "ds")->add_child_text (x509_serial_number);
	}

	string x509_issuer_name;
	string x509_serial_number;
};

class Recipient
{
public:
	Recipient () {}
	
	Recipient (shared_ptr<const cxml::Node> node)
		: x509_issuer_serial (node->node_child ("X509IssuerSerial"))
		, x509_subject_name (node->string_child ("X509SubjectName"))
	{

	}

	void as_xml (xmlpp::Element* node) const
	{
		x509_issuer_serial.as_xml (node->add_child ("X509IssuerSerial"));
		node->add_child("X509SubjectName")->add_child_text (x509_subject_name);
	}
	
	X509IssuerSerial x509_issuer_serial;
	string x509_subject_name;
};

class KDMRequiredExtensions
{
public:
	KDMRequiredExtensions () {}
	
	KDMRequiredExtensions (shared_ptr<const cxml::Node> node)
		: recipient (node->node_child ("Recipient"))
		, composition_playlist_id (node->string_child ("CompositionPlaylistId").substr (9))
		, content_title_text (node->string_child ("ContentTitleText"))
		, not_valid_before (node->string_child ("ContentKeysNotValidBefore"))
		, not_valid_after (node->string_child ("ContentKeysNotValidAfter"))
		, authorized_device_info (node->node_child ("AuthorizedDeviceInfo"))
		, key_id_list (node->node_child ("KeyIdList"))
	{

	}

	void as_xml (xmlpp::Element* node) const
	{
		node->set_attribute ("xmlns", "http://www.smpte-ra.org/schemas/430-1/2006/KDM");
		
		recipient.as_xml (node->add_child ("Recipient"));
		node->add_child("CompositionPlaylistId")->add_child_text ("urn:uuid:" + composition_playlist_id);
		if (content_authenticator) {
			node->add_child("ContentAuthenticator")->add_child_text (content_authenticator.get ());
		}
		node->add_child("ContentTitleText")->add_child_text (content_title_text);
		node->add_child("ContentKeysNotValidBefore")->add_child_text (not_valid_before.as_string ());
		node->add_child("ContentKeysNotValidAfter")->add_child_text (not_valid_after.as_string ());
		authorized_device_info.as_xml (node->add_child ("AuthorizedDeviceInfo"));
		key_id_list.as_xml (node->add_child ("KeyIdList"));
		
		xmlpp::Element* forensic_mark_flag_list = node->add_child ("ForensicMarkFlagList");
		forensic_mark_flag_list->add_child("ForensicMarkFlag")->add_child_text ("http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-picture-disable");
		forensic_mark_flag_list->add_child("ForensicMarkFlag")->add_child_text ("http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-audio-disable");
	}
	
	Recipient recipient;
	string composition_playlist_id;
	boost::optional<string> content_authenticator;
	string content_title_text;
	LocalTime not_valid_before;
	LocalTime not_valid_after;
	AuthorizedDeviceInfo authorized_device_info;
	KeyIdList key_id_list;
};

class RequiredExtensions
{
public:
	RequiredExtensions () {}
	
	RequiredExtensions (shared_ptr<const cxml::Node> node)
		: kdm_required_extensions (node->node_child ("KDMRequiredExtensions"))
	{

	}

	void as_xml (xmlpp::Element* node) const
	{
		kdm_required_extensions.as_xml (node->add_child ("KDMRequiredExtensions"));
	}
	
	KDMRequiredExtensions kdm_required_extensions;
};

class AuthenticatedPublic
{
public:
	AuthenticatedPublic ()
		: message_id (make_uuid ())
		, issue_date (LocalTime().as_string ())
	{}
	
	AuthenticatedPublic (shared_ptr<const cxml::Node> node)
		: message_id (node->string_child ("MessageId").substr (9))
		, annotation_text (node->string_child ("AnnotationText"))
		, issue_date (node->string_child ("IssueDate"))
		, signer (node->node_child ("Signer"))
		, required_extensions (node->node_child ("RequiredExtensions"))
	{

	}

	void as_xml (xmlpp::Element* node, map<string, xmlpp::Attribute *>& references) const
	{
		references["ID_AuthenticatedPublic"] = node->set_attribute ("Id", "ID_AuthenticatedPublic");
		
		node->add_child("MessageId")->add_child_text ("urn:uuid:" + message_id);
		node->add_child("MessageType")->add_child_text ("http://www.smpte-ra.org/430-1/2006/KDM#kdm-key-type");
		node->add_child("AnnotationText")->add_child_text (annotation_text);
		node->add_child("IssueDate")->add_child_text (issue_date);

		signer.as_xml (node->add_child ("Signer"));
		required_extensions.as_xml (node->add_child ("RequiredExtensions"));

		node->add_child ("NonCriticalExtensions");
	}

	string message_id;
	string annotation_text;
	string issue_date;
	Signer signer;
	RequiredExtensions required_extensions;
};

/** Class to describe our data.  We use a class hierarchy as it's a bit nicer
 *  for XML data than a flat description.
 */
class EncryptedKDMData
{
public:
	EncryptedKDMData ()
	{

	}
	
	EncryptedKDMData (shared_ptr<const cxml::Node> node)
		: authenticated_public (node->node_child ("AuthenticatedPublic"))
		, authenticated_private (node->node_child ("AuthenticatedPrivate"))
		, signature (node->node_child ("Signature"))
	{
		
	}

	shared_ptr<xmlpp::Document> as_xml () const
	{
		shared_ptr<xmlpp::Document> document (new xmlpp::Document ());
		xmlpp::Element* root = document->create_root_node ("DCinemaSecurityMessage", "http://www.smpte-ra.org/schemas/430-3/2006/ETM");
		root->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "ds");
		root->set_namespace_declaration ("http://www.w3.org/2001/04/xmlenc#", "enc");
		map<string, xmlpp::Attribute *> references;
		authenticated_public.as_xml (root->add_child ("AuthenticatedPublic"), references);
		authenticated_private.as_xml (root->add_child ("AuthenticatedPrivate"), references);
		signature.as_xml (root->add_child ("Signature", "ds"));

		for (map<string, xmlpp::Attribute*>::const_iterator i = references.begin(); i != references.end(); ++i) {
			xmlAddID (0, document->cobj(), (const xmlChar *) i->first.c_str(), i->second->cobj ());
		}

		return document;
	}

	AuthenticatedPublic authenticated_public;
	AuthenticatedPrivate authenticated_private;
	Signature signature;
};

}
}

EncryptedKDM::EncryptedKDM (string s)
{
	shared_ptr<cxml::Document> doc (new cxml::Document ("DCinemaSecurityMessage"));
	doc->read_string (s);
	_data = new data::EncryptedKDMData (doc);
}

EncryptedKDM::EncryptedKDM (
	shared_ptr<const Signer> signer,
	Certificate recipient,
	string device_list_description,
	string cpl_id,
	string content_title_text,
	LocalTime not_valid_before,
	LocalTime not_valid_after,
	Formulation formulation,
	list<pair<string, string> > key_ids,
	list<string> keys
	)
	: _data (new data::EncryptedKDMData)
{
	/* Fill our XML-ish description in with the juicy bits that the caller has given */
	
	data::AuthenticatedPublic& aup = _data->authenticated_public;
	aup.signer.x509_issuer_name = signer->certificates().leaf().issuer ();
	aup.signer.x509_serial_number = signer->certificates().leaf().serial ();

	data::KDMRequiredExtensions& kre = _data->authenticated_public.required_extensions.kdm_required_extensions;
	kre.recipient.x509_issuer_serial.x509_issuer_name = recipient.issuer ();
	kre.recipient.x509_issuer_serial.x509_serial_number = recipient.serial ();
	kre.recipient.x509_subject_name = recipient.subject ();
	kre.authorized_device_info.device_list_description = device_list_description;
	kre.composition_playlist_id = cpl_id;
	if (formulation == DCI_ANY || formulation == DCI_SPECIFIC) {
		kre.content_authenticator = signer->certificates().leaf().thumbprint ();
	}
	kre.content_title_text = content_title_text;
	kre.not_valid_before = not_valid_before;
	kre.not_valid_after = not_valid_after;
	kre.authorized_device_info.device_list_identifier = make_uuid ();
	string n = recipient.common_name ();
	if (n.find (".") != string::npos) {
		n = n.substr (n.find (".") + 1);
	}
	kre.authorized_device_info.device_list_description = n;

	if (formulation == MODIFIED_TRANSITIONAL_1 || formulation == DCI_ANY) {
		/* Use the "assume trust" thumbprint */
		kre.authorized_device_info.certificate_thumbprint = "2jmj7l5rSw0yVb/vlWAYkK/YBwk=";
	} else if (formulation == DCI_SPECIFIC) {
		/* Use the recipient thumbprint */
		kre.authorized_device_info.certificate_thumbprint = recipient.thumbprint ();
	}

	for (list<pair<string, string> >::const_iterator i = key_ids.begin(); i != key_ids.end(); ++i) {
		kre.key_id_list.typed_key_id.push_back (data::TypedKeyId (i->first, i->second));
	}

	_data->authenticated_private.encrypted_key = keys;

	/* Read the XML so far and sign it */
	shared_ptr<xmlpp::Document> doc = _data->as_xml ();
	xmlpp::Node::NodeList children = doc->get_root_node()->get_children ();
	for (xmlpp::Node::NodeList::const_iterator i = children.begin(); i != children.end(); ++i) {
		if ((*i)->get_name() == "Signature") {
			signer->add_signature_value (*i, "ds");
		}
	}

	/* Read the bits that add_signature_value did back into our variables */
	shared_ptr<cxml::Node> signed_doc (new cxml::Node (doc->get_root_node ()));
	_data->signature = data::Signature (signed_doc->node_child ("Signature"));
}

EncryptedKDM::EncryptedKDM (EncryptedKDM const & other)
	: _data (new data::EncryptedKDMData (*other._data))
{

}

EncryptedKDM &
EncryptedKDM::operator= (EncryptedKDM const & other)
{
	if (this == &other) {
		return *this;
	}

	delete _data;
	_data = new data::EncryptedKDMData (*other._data);
	return *this;
}

EncryptedKDM::~EncryptedKDM ()
{
	delete _data;
}

void
EncryptedKDM::as_xml (boost::filesystem::path path) const
{
	FILE* f = fopen_boost (path, "w");
	string const x = as_xml ();
	fwrite (x.c_str(), 1, x.length(), f);
	fclose (f);
}
       
string
EncryptedKDM::as_xml () const
{
	xmlpp::Document document;
	xmlpp::Element* root = document.create_root_node ("DCinemaSecurityMessage", "http://www.smpte-ra.org/schemas/430-3/2006/ETM");
	root->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "ds");
	root->set_namespace_declaration ("http://www.w3.org/2001/04/xmlenc#", "enc");

	return _data->as_xml()->write_to_string ("UTF-8");
}

list<string>
EncryptedKDM::keys () const
{
	return _data->authenticated_private.encrypted_key;
}
