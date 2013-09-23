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

#include <libxml++/libxml++.h>
#include <libcxml/cxml.h>
#include "kdm_smpte.h"

using std::list;
using std::string;
using boost::shared_ptr;
using namespace libdcp::xml;

DCinemaSecurityMessage::DCinemaSecurityMessage (boost::filesystem::path file)
{
	cxml::Document f ("DCinemaSecurityMessage");
	f.read_file (file.string ());

	authenticated_public = AuthenticatedPublic (f.node_child ("AuthenticatedPublic"));
	authenticated_private = AuthenticatedPrivate (f.node_child ("AuthenticatedPrivate"));
	signature = Signature (f.node_child ("Signature"));

	f.done ();
}

void
DCinemaSecurityMessage::as_xml (boost::filesystem::path file) const
{
	xmlpp::Document doc;
	xmlpp::Element* root = doc.create_root_node ("DCinemaSecurityMessage", "http://www.smpte-ra.org/schemas/430-3/2006/ETM");
	root->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "ds");
	root->set_namespace_declaration ("http://www.w3.org/2001/04/xmlenc#", "enc");

	authenticated_public.as_xml (root->add_child ("AuthenticatedPublic"));
	authenticated_private.as_xml (root->add_child ("AuthenticatedPrivate"));
	signature.as_xml (root->add_child ("Signature", "ds"));

	doc.write_to_file_formatted (file.string (), "UTF-8");
}

AuthenticatedPublic::AuthenticatedPublic (shared_ptr<const cxml::Node> node)
	: message_id (node->string_child ("MessageId"))
	, message_type (node->string_child ("MessageType"))
	, annotation_text (node->optional_string_child ("AnnotationText"))
	, issue_date (node->string_child ("IssueDate"))
	, signer (node->node_child ("Signer"))
{
	shared_ptr<const cxml::Node> c = node->node_child ("RequiredExtensions");
	c = c->node_child ("KDMRequiredExtensions");
	recipient = Recipient (c->node_child ("Recipient"));
	composition_playlist_id = c->string_child ("CompositionPlaylistId");
	content_title_text = c->string_child ("ContentTitleText");
	content_keys_not_valid_before = c->string_child ("ContentKeysNotValidBefore");
	content_keys_not_valid_after = c->string_child ("ContentKeysNotValidAfter");
	authorized_device_info = AuthorizedDeviceInfo (c->node_child ("AuthorizedDeviceInfo"));

	list<shared_ptr<cxml::Node> > kil = c->node_child("KeyIdList")->node_children("TypedKeyId");
	for (list<shared_ptr<cxml::Node> >::iterator i = kil.begin(); i != kil.end(); ++i) {
		key_id_list.push_back (TypedKeyId (*i));
	}

	list<shared_ptr<cxml::Node> > fmf = c->node_child("ForensicMarkFlagList")->node_children("ForensicMarkFlag");
	for (list<shared_ptr<cxml::Node> >::iterator i = fmf.begin(); i != fmf.end(); ++i) {
		forensic_mark_flag_list.push_back ((*i)->content ());
	}

	node->ignore_child ("NonCriticalExtensions");
	node->done ();
}

void
AuthenticatedPublic::as_xml (xmlpp::Element* node) const
{
	node->set_attribute ("Id", "ID_AuthenticatedPublic");
	
	node->add_child("MessageId")->add_child_text (message_id);
	node->add_child("MessageType")->add_child_text (message_type);
	if (annotation_text) {
		node->add_child("AnnotationText")->add_child_text (annotation_text.get ());
	}
	node->add_child("IssueDate")->add_child_text (issue_date);
	signer.as_xml (node->add_child("Signer"));
	
	xmlpp::Element* kdm_required_extensions = node->add_child("RequiredExtensions")->add_child("KDMRequiredExtensions");
	kdm_required_extensions->set_attribute ("xmlns", "http://www.smpte-ra.org/schemas/430-1/2006/KDM");
	recipient.as_xml (kdm_required_extensions->add_child ("Recipient"));
	
	kdm_required_extensions->add_child("CompositionPlaylistId")->add_child_text (composition_playlist_id);
	kdm_required_extensions->add_child("ContentTitleText")->add_child_text (content_title_text);
	kdm_required_extensions->add_child("ContentKeysNotValidBefore")->add_child_text (content_keys_not_valid_before);
	kdm_required_extensions->add_child("ContentKeysNotValidAfter")->add_child_text (content_keys_not_valid_after);
	authorized_device_info.as_xml (kdm_required_extensions->add_child("AuthorizedDeviceInfo"));

	xmlpp::Element* kil = kdm_required_extensions->add_child("KeyIdList");
	for (list<TypedKeyId>::const_iterator i = key_id_list.begin(); i != key_id_list.end(); ++i) {
		i->as_xml (kil->add_child ("TypedKeyId"));
	}

	xmlpp::Element* fmfl = kdm_required_extensions->add_child ("ForensicMarkFlagList");
	for (list<string>::const_iterator i = forensic_mark_flag_list.begin(); i != forensic_mark_flag_list.end(); ++i) {
		fmfl->add_child("ForensicMarkFlag")->add_child_text (*i);
	}

	node->add_child ("NonCriticalExtensions");
}

Signer::Signer (shared_ptr<const cxml::Node> node)
	: x509_issuer_name (node->string_child ("X509IssuerName"))
	, x509_serial_number (node->string_child ("X509SerialNumber"))
{
	node->done ();
}

void
Signer::as_xml (xmlpp::Element* node) const
{
	node->add_child("X509IssuerName", "ds")->add_child_text (x509_issuer_name);
	node->add_child("X509SerialNumber", "ds")->add_child_text (x509_serial_number);
}

Recipient::Recipient (shared_ptr<const cxml::Node> node)
	: x509_issuer_serial (node->node_child ("X509IssuerSerial"))
	, x509_subject_name (node->string_child ("X509SubjectName"))
{
	node->done ();
}

void
Recipient::as_xml (xmlpp::Element* node) const
{
	x509_issuer_serial.as_xml (node->add_child ("X509IssuerSerial"));
	node->add_child("X509SubjectName")->add_child_text (x509_subject_name);
}
				     
AuthorizedDeviceInfo::AuthorizedDeviceInfo (shared_ptr<const cxml::Node> node)
	: device_list_identifier (node->string_child ("DeviceListIdentifier"))
	, device_list_description (node->string_child ("DeviceListDescription"))
{
	list<shared_ptr<cxml::Node> > ct = node->node_child("DeviceList")->node_children("CertificateThumbprint");
	for (list<shared_ptr<cxml::Node> >::const_iterator i = ct.begin(); i != ct.end(); ++i) {
		device_list.push_back ((*i)->content ());
	}

	node->done ();
}

void
AuthorizedDeviceInfo::as_xml (xmlpp::Element* node) const
{
	node->add_child ("DeviceListIdentifier")->add_child_text (device_list_identifier);
	node->add_child ("DeviceListDescription")->add_child_text (device_list_description);
	xmlpp::Element* dl = node->add_child ("DeviceList");
	for (list<string>::const_iterator i = device_list.begin(); i != device_list.end(); ++i) {
		dl->add_child("CertificateThumbprint")->add_child_text (*i);
	}
}

TypedKeyId::TypedKeyId (shared_ptr<const cxml::Node> node)
	: key_type (node->string_child ("KeyType"))
	, key_id (node->string_child ("KeyId"))
{
	node->done ();
}

void
TypedKeyId::as_xml (xmlpp::Element* node) const
{
	node->add_child("KeyType")->add_child_text (key_type);
	node->add_child("KeyId")->add_child_text (key_id);
}

AuthenticatedPrivate::AuthenticatedPrivate (shared_ptr<const cxml::Node> node)
{
	list<shared_ptr<cxml::Node> > ek = node->node_children ("EncryptedKey");
	for (list<shared_ptr<cxml::Node> >::const_iterator i = ek.begin(); i != ek.end(); ++i) {
		encrypted_keys.push_back ((*i)->node_child("CipherData")->string_child("CipherValue"));
	}

	node->done ();
}

void
AuthenticatedPrivate::as_xml (xmlpp::Element* node) const
{
	node->set_attribute ("Id", "ID_AuthenticatedPrivate");
	
	for (list<string>::const_iterator i = encrypted_keys.begin(); i != encrypted_keys.end(); ++i) {
		xmlpp::Element* encrypted_key = node->add_child ("EncryptedKey", "enc");
		xmlpp::Element* encryption_method = encrypted_key->add_child ("EncryptionMethod", "enc");
		encryption_method->set_attribute ("Algorithm", "http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p");
		xmlpp::Element* digest_method = encryption_method->add_child ("DigestMethod", "ds");
		digest_method->set_attribute ("Algorithm", "http://www.w3.org/2000/09/xmldsig#sha1");
		xmlpp::Element* cipher_data = encrypted_key->add_child ("CipherData", "enc");
		cipher_data->add_child("CipherValue", "enc")->add_child_text (*i);
	}
}

Signature::Signature (shared_ptr<const cxml::Node> node)
	: signature_value (node->string_child ("SignatureValue"))
{
	list<shared_ptr<cxml::Node> > refs = node->node_child("SignedInfo")->node_children ("Reference");
	for (list<shared_ptr<cxml::Node> >::const_iterator i = refs.begin(); i != refs.end(); ++i) {
		signed_info.push_back (Reference (*i));
	}

	list<shared_ptr<cxml::Node> > data = node->node_child("KeyInfo")->node_children ("X509Data");
	for (list<shared_ptr<cxml::Node> >::const_iterator i = data.begin(); i != data.end(); ++i) {
		key_info.push_back (X509Data (*i));
	}
	
	node->done ();
}

void
Signature::as_xml (xmlpp::Element* node) const
{
	xmlpp::Element* si = node->add_child ("SignedInfo", "ds");
	si->add_child ("CanonicalizationMethod", "ds")->set_attribute ("Algorithm", "http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments");
	si->add_child ("SignatureMethod", "ds")->set_attribute ("Algorithm", "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256");
	for (list<Reference>::const_iterator i = signed_info.begin(); i != signed_info.end(); ++i) {
		i->as_xml (si);
	}
	
	node->add_child("SignatureValue", "ds")->add_child_text (signature_value);

	xmlpp::Element* ki = node->add_child ("KeyInfo", "ds");
	for (list<X509Data>::const_iterator i = key_info.begin(); i != key_info.end(); ++i) {
		i->as_xml (ki->add_child ("X509Data", "ds"));
	}
}

Reference::Reference (shared_ptr<const cxml::Node> node)
{
	uri = node->string_attribute ("URI");
	digest_value = node->string_child ("DigestValue");
	
	node->ignore_child ("DigestMethod");
	node->done ();
}

void
Reference::as_xml (xmlpp::Element* node) const
{
	xmlpp::Element* reference = node->add_child ("Reference", "ds");
	reference->set_attribute ("URI", uri);
	reference->add_child("DigestMethod", "ds")->set_attribute ("Algorithm", "http://www.w3.org/2001/04/xmlenc#sha256");
	reference->add_child("DigestValue", "ds")->add_child_text (digest_value);
}

X509Data::X509Data (shared_ptr<const cxml::Node> node)
{
	x509_issuer_serial = Signer (node->node_child ("X509IssuerSerial"));
	x509_certificate = node->string_child ("X509Certificate");

	node->done ();
}

void
X509Data::as_xml (xmlpp::Element* node) const
{
	x509_issuer_serial.as_xml (node->add_child ("X509IssuerSerial", "ds"));
	node->add_child("X509Certificate", "ds")->add_child_text (x509_certificate);
}
