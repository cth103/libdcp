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

/** @file src/xml/kdm_smpte.h
 *  @brief 1:1ish C++ representations of the XML schema for a SMPTE KDM.
 *
 *  This file contains classes which map pretty-much 1:1 to the elements in a SMPTE KDM
 *  (Key Delivery Message).  The `main' KDM class contains a pointer to a DCinemaSecurityMessage
 *  from this file.
 *
 *  This should probably have been automatically generated from the XSD,
 *  but I think it's too much trouble considering that the XSD does not
 *  change very often.
 */

#ifndef LIBDCP_XML_KDM_SMPTE_H
#define LIBDCP_XML_KDM_SMPTE_H

#include <string>
#include <list>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <libxml/parser.h>
#include <libxml++/libxml++.h>
#include <libcxml/cxml.h>
#include "../exceptions.h"

namespace libdcp {
namespace xml {

class Writer
{
public:
	Writer ()
		: document (new xmlpp::Document)
	{}
	
	boost::shared_ptr<xmlpp::Document> document;
	std::map<std::string, xmlpp::Attribute *> references;
};

class Signer
{
public:
	Signer () {}
	Signer (boost::shared_ptr<const cxml::Node> node)
		: x509_issuer_name (node->string_child ("X509IssuerName"))
		, x509_serial_number (node->string_child ("X509SerialNumber"))
	{
		node->done ();
	}

	void as_xml (xmlpp::Element* node) const
	{
		node->add_child("X509IssuerName", "ds")->add_child_text (x509_issuer_name);
		node->add_child("X509SerialNumber", "ds")->add_child_text (x509_serial_number);
	}
	
	std::string x509_issuer_name;
	std::string x509_serial_number;
};

class Recipient
{
public:
	Recipient () {}
	Recipient (boost::shared_ptr<const cxml::Node> node)
		: x509_issuer_serial (node->node_child ("X509IssuerSerial"))
		, x509_subject_name (node->string_child ("X509SubjectName"))
	{
		node->done ();
	}

	void as_xml (xmlpp::Element* node) const
	{
		x509_issuer_serial.as_xml (node->add_child ("X509IssuerSerial"));
		node->add_child("X509SubjectName")->add_child_text (x509_subject_name);
	}
	
	Signer x509_issuer_serial;
	std::string x509_subject_name;
};

class AuthorizedDeviceInfo
{
public:
	AuthorizedDeviceInfo () {}
	AuthorizedDeviceInfo (boost::shared_ptr<const cxml::Node> node)
		: device_list_identifier (node->string_child ("DeviceListIdentifier"))
		, device_list_description (node->string_child ("DeviceListDescription"))
	{
		std::list<boost::shared_ptr<cxml::Node> > ct = node->node_child("DeviceList")->node_children("CertificateThumbprint");
		for (std::list<boost::shared_ptr<cxml::Node> >::const_iterator i = ct.begin(); i != ct.end(); ++i) {
			device_list.push_back ((*i)->content ());
		}

		node->done ();
	}
	
	void as_xml (xmlpp::Element* node) const
	{
		node->add_child ("DeviceListIdentifier")->add_child_text (device_list_identifier);
		node->add_child ("DeviceListDescription")->add_child_text (device_list_description);
		xmlpp::Element* dl = node->add_child ("DeviceList");
		for (std::list<std::string>::const_iterator i = device_list.begin(); i != device_list.end(); ++i) {
			dl->add_child("CertificateThumbprint")->add_child_text (*i);
		}
	}
	
	std::string device_list_identifier;
	std::string device_list_description;
	std::list<std::string> device_list;
};

class TypedKeyId
{
public:
	TypedKeyId () {}

	TypedKeyId (std::string t, std::string i)
		: key_type (t)
		, key_id (i)
	{}
	
	TypedKeyId (boost::shared_ptr<const cxml::Node> node)
		: key_type (node->string_child ("KeyType"))
		, key_id (node->string_child ("KeyId"))
	{
		node->done ();
	}
	
	void as_xml (xmlpp::Element* node) const
	{
		node->add_child("KeyType")->add_child_text (key_type);
		node->add_child("KeyId")->add_child_text (key_id);
	}

	std::string key_type;
	std::string key_id;
};

class AuthenticatedPublic
{
public:
	AuthenticatedPublic () {}
	AuthenticatedPublic (boost::shared_ptr<const cxml::Node> node)
		: message_id (node->string_child ("MessageId"))
		, message_type (node->string_child ("MessageType"))
		, annotation_text (node->optional_string_child ("AnnotationText"))
		, issue_date (node->string_child ("IssueDate"))
		, signer (node->node_child ("Signer"))
	{
		boost::shared_ptr<const cxml::Node> c = node->node_child ("RequiredExtensions");
		c = c->node_child ("KDMRequiredExtensions");
		recipient = Recipient (c->node_child ("Recipient"));
		composition_playlist_id = c->string_child ("CompositionPlaylistId");
		content_authenticator = c->optional_string_child ("ContentAuthenticator");
		content_title_text = c->string_child ("ContentTitleText");
		content_keys_not_valid_before = c->string_child ("ContentKeysNotValidBefore");
		content_keys_not_valid_after = c->string_child ("ContentKeysNotValidAfter");
		authorized_device_info = AuthorizedDeviceInfo (c->node_child ("AuthorizedDeviceInfo"));
		
		std::list<boost::shared_ptr<cxml::Node> > kil = c->node_child("KeyIdList")->node_children("TypedKeyId");
		for (std::list<boost::shared_ptr<cxml::Node> >::iterator i = kil.begin(); i != kil.end(); ++i) {
			key_id_list.push_back (TypedKeyId (*i));
		}
		
		boost::shared_ptr<cxml::Node> fmfl = c->optional_node_child("ForensicMarkFlagList");
		if (fmfl) {
			std::list<boost::shared_ptr<cxml::Node> > fmf = fmfl->node_children("ForensicMarkFlag");
			for (std::list<boost::shared_ptr<cxml::Node> >::iterator i = fmf.begin(); i != fmf.end(); ++i) {
				forensic_mark_flag_list.push_back ((*i)->content ());
			}
		}
		
		node->ignore_child ("NonCriticalExtensions");
		node->done ();
	}

	void as_xml (Writer& writer, xmlpp::Element* node) const
	{
		writer.references["ID_AuthenticatedPublic"] = node->set_attribute ("Id", "ID_AuthenticatedPublic");
		
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
		if (content_authenticator) {
			kdm_required_extensions->add_child("ContentAuthenticator")->add_child_text (content_authenticator.get ());
		}
		kdm_required_extensions->add_child("ContentTitleText")->add_child_text (content_title_text);
		kdm_required_extensions->add_child("ContentKeysNotValidBefore")->add_child_text (content_keys_not_valid_before);
		kdm_required_extensions->add_child("ContentKeysNotValidAfter")->add_child_text (content_keys_not_valid_after);
		authorized_device_info.as_xml (kdm_required_extensions->add_child("AuthorizedDeviceInfo"));
		
		xmlpp::Element* kil = kdm_required_extensions->add_child("KeyIdList");
		for (std::list<TypedKeyId>::const_iterator i = key_id_list.begin(); i != key_id_list.end(); ++i) {
			i->as_xml (kil->add_child ("TypedKeyId"));
		}
		
		xmlpp::Element* fmfl = kdm_required_extensions->add_child ("ForensicMarkFlagList");
		for (std::list<std::string>::const_iterator i = forensic_mark_flag_list.begin(); i != forensic_mark_flag_list.end(); ++i) {
			fmfl->add_child("ForensicMarkFlag")->add_child_text (*i);
		}
		
		node->add_child ("NonCriticalExtensions");
	}
	
	std::string message_id;
	std::string message_type;
	boost::optional<std::string> annotation_text;
	std::string issue_date;
	Signer signer;
	Recipient recipient;
	std::string composition_playlist_id;
	boost::optional<std::string> content_authenticator;
	std::string content_title_text;
	std::string content_keys_not_valid_before;
	std::string content_keys_not_valid_after;
	AuthorizedDeviceInfo authorized_device_info;
	std::list<TypedKeyId> key_id_list;
	std::list<std::string> forensic_mark_flag_list;
};

class AuthenticatedPrivate
{
public:
	AuthenticatedPrivate () {}
	
	AuthenticatedPrivate (boost::shared_ptr<const cxml::Node> node)
	{
		std::list<boost::shared_ptr<cxml::Node> > ek = node->node_children ("EncryptedKey");
		for (std::list<boost::shared_ptr<cxml::Node> >::const_iterator i = ek.begin(); i != ek.end(); ++i) {
			encrypted_keys.push_back ((*i)->node_child("CipherData")->string_child("CipherValue"));
		}
		
		node->done ();
	}
	
	void as_xml (Writer& writer, xmlpp::Element* node) const
	{
		writer.references["ID_AuthenticatedPrivate"] = node->set_attribute ("Id", "ID_AuthenticatedPrivate");
		
		for (std::list<std::string>::const_iterator i = encrypted_keys.begin(); i != encrypted_keys.end(); ++i) {
			xmlpp::Element* encrypted_key = node->add_child ("EncryptedKey", "enc");
			xmlpp::Element* encryption_method = encrypted_key->add_child ("EncryptionMethod", "enc");
			encryption_method->set_attribute ("Algorithm", "http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p");
			xmlpp::Element* digest_method = encryption_method->add_child ("DigestMethod", "ds");
			digest_method->set_attribute ("Algorithm", "http://www.w3.org/2000/09/xmldsig#sha1");
			xmlpp::Element* cipher_data = encrypted_key->add_child ("CipherData", "enc");
			cipher_data->add_child("CipherValue", "enc")->add_child_text (*i);
		}
	}	
	
	std::list<std::string> encrypted_keys;
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
	Reference (std::string u)
		: uri (u)
	{}
		  
	Reference (boost::shared_ptr<const cxml::Node> node)
		: uri (node->string_attribute ("URI"))
		, digest_value (node->string_child ("DigestValue"))
	{
		node->ignore_child ("DigestMethod");
		node->done ();
	}

	void as_xml (xmlpp::Element* node) const
	{
		xmlpp::Element* reference = node->add_child ("Reference", "ds");
		reference->set_attribute ("URI", uri);
		reference->add_child("DigestMethod", "ds")->set_attribute ("Algorithm", "http://www.w3.org/2001/04/xmlenc#sha256");
		reference->add_child("DigestValue", "ds")->add_child_text (digest_value);
	}
	
	std::string uri;
	std::string digest_value;
};

class Signature
{
public:
	Signature ()
		: authenticated_public ("#ID_AuthenticatedPublic")
		, authenticated_private ("#ID_AuthenticatedPrivate")
	{}
	
	Signature (boost::shared_ptr<const cxml::Node> node)
	{
		std::list<boost::shared_ptr<cxml::Node> > refs = node->node_child("SignedInfo")->node_children ("Reference");
		for (std::list<boost::shared_ptr<cxml::Node> >::const_iterator i = refs.begin(); i != refs.end(); ++i) {
			if ((*i)->string_attribute("URI") == "#ID_AuthenticatedPublic") {
				authenticated_public = Reference (*i);
			} else if ((*i)->string_attribute("URI") == "#ID_AuthenticatedPrivate") {
				authenticated_private = Reference (*i);
			} else {
				throw XMLError ("unrecognised reference URI");
			}
		}
		
		std::list<boost::shared_ptr<cxml::Node> > data = node->node_child("KeyInfo")->node_children ("X509Data");
		for (std::list<boost::shared_ptr<cxml::Node> >::const_iterator i = data.begin(); i != data.end(); ++i) {
			key_info.push_back (X509Data (*i));
		}

		signature_value = node->string_child ("SignatureValue");
		
		node->done ();
	}
	
	void as_xml (xmlpp::Element* node) const
	{
		xmlpp::Element* si = node->add_child ("SignedInfo", "ds");
		si->add_child ("CanonicalizationMethod", "ds")->set_attribute ("Algorithm", "http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments");
		si->add_child ("SignatureMethod", "ds")->set_attribute ("Algorithm", "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256");
		
		authenticated_public.as_xml (si);
		authenticated_private.as_xml (si);
		
		node->add_child("SignatureValue", "ds")->add_child_text (signature_value);
		
		xmlpp::Element* ki = node->add_child ("KeyInfo", "ds");
		for (std::list<X509Data>::const_iterator i = key_info.begin(); i != key_info.end(); ++i) {
			i->as_xml (ki->add_child ("X509Data", "ds"));
		}
	}

	Reference authenticated_public;
	Reference authenticated_private;
	std::string signature_value;
	std::list<X509Data> key_info;
};

class DCinemaSecurityMessage
{
public:
	DCinemaSecurityMessage () {}
	DCinemaSecurityMessage (boost::filesystem::path file)
	{
		cxml::Document f ("DCinemaSecurityMessage");
		f.read_file (file.string ());
		
		authenticated_public = AuthenticatedPublic (f.node_child ("AuthenticatedPublic"));
		authenticated_private = AuthenticatedPrivate (f.node_child ("AuthenticatedPrivate"));
		signature = Signature (f.node_child ("Signature"));
		
		f.done ();
	}
	
	boost::shared_ptr<xmlpp::Document> as_xml () const
	{
		Writer writer;
		
		xmlpp::Element* root = writer.document->create_root_node ("DCinemaSecurityMessage", "http://www.smpte-ra.org/schemas/430-3/2006/ETM");
		root->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "ds");
		root->set_namespace_declaration ("http://www.w3.org/2001/04/xmlenc#", "enc");
		
		authenticated_public.as_xml (writer, root->add_child ("AuthenticatedPublic"));
		authenticated_private.as_xml (writer, root->add_child ("AuthenticatedPrivate"));
		signature.as_xml (root->add_child ("Signature", "ds"));

		for (std::map<std::string, xmlpp::Attribute*>::const_iterator i = writer.references.begin(); i != writer.references.end(); ++i) {
			xmlAddID (0, writer.document->cobj(), (const xmlChar *) i->first.c_str(), i->second->cobj ());
		}
		
		return writer.document;
	}

	AuthenticatedPublic authenticated_public;
	AuthenticatedPrivate authenticated_private;
	Signature signature;
};

}
}

#endif
	
