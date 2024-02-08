/*
    Copyright (C) 2013-2021 Carl Hetherington <cth@carlh.net>

    This file is part of libdcp.

    libdcp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libdcp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdcp.  If not, see <http://www.gnu.org/licenses/>.

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/


/** @file  src/encrypted_kdm.cc
 *  @brief EncryptedKDM class
 */


#include "certificate_chain.h"
#include "compose.hpp"
#include "encrypted_kdm.h"
#include "exceptions.h"
#include "file.h"
#include "util.h"
#include <libcxml/cxml.h>
#include <libxml++/document.h>
#include <libxml++/nodes/element.h>
#include <libxml/parser.h>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>


using std::list;
using std::vector;
using std::string;
using std::make_shared;
using std::map;
using std::pair;
using std::shared_ptr;
using boost::optional;
using boost::starts_with;
using namespace dcp;


namespace dcp {


/** Namespace for classes used to hold our data; they are internal to this .cc file */
namespace data {


class Signer
{
public:
	Signer () {}

	explicit Signer (shared_ptr<const cxml::Node> node)
		: x509_issuer_name (node->string_child ("X509IssuerName"))
		, x509_serial_number (node->string_child ("X509SerialNumber"))
	{

	}

	void as_xml (xmlpp::Element* node) const
	{
		cxml::add_child(node, "X509IssuerName", string("ds"))->add_child_text(x509_issuer_name);
		cxml::add_child(node, "X509SerialNumber", string("ds"))->add_child_text(x509_serial_number);
	}

	string x509_issuer_name;
	string x509_serial_number;
};


class X509Data
{
public:
	X509Data () {}

	explicit X509Data (std::shared_ptr<const cxml::Node> node)
		: x509_issuer_serial (Signer (node->node_child ("X509IssuerSerial")))
		, x509_certificate (node->string_child ("X509Certificate"))
	{
		node->done ();
	}

	void as_xml (xmlpp::Element* node) const
	{
		x509_issuer_serial.as_xml(cxml::add_child(node, "X509IssuerSerial", string("ds")));
		cxml::add_child(node, "X509Certificate", string("ds"))->add_child_text(x509_certificate);
	}

	Signer x509_issuer_serial;
	std::string x509_certificate;
};


class Reference
{
public:
	Reference () {}

	explicit Reference (string u)
		: uri (u)
	{}

	explicit Reference (shared_ptr<const cxml::Node> node)
		: uri (node->string_attribute ("URI"))
		, digest_value (node->string_child ("DigestValue"))
	{

	}

	void as_xml (xmlpp::Element* node) const
	{
		node->set_attribute ("URI", uri);
		cxml::add_child(node, "DigestMethod", string("ds"))->set_attribute("Algorithm", "http://www.w3.org/2001/04/xmlenc#sha256");
		cxml::add_child(node, "DigestValue", string("ds"))->add_child_text(digest_value);
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

	explicit SignedInfo (shared_ptr<const cxml::Node> node)
	{
		for (auto i: node->node_children ("Reference")) {
			if (i->string_attribute("URI") == "#ID_AuthenticatedPublic") {
				authenticated_public = Reference(i);
			} else if (i->string_attribute("URI") == "#ID_AuthenticatedPrivate") {
				authenticated_private = Reference(i);
			}

			/* XXX: do something if we don't recognise the node */
		}
	}

	void as_xml (xmlpp::Element* node) const
	{
		cxml::add_child(node, "CanonicalizationMethod", string("ds"))->set_attribute(
			"Algorithm", "http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments"
			);

		cxml::add_child(node, "SignatureMethod", string("ds"))->set_attribute(
			"Algorithm", "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256"
			);

		authenticated_public.as_xml(cxml::add_child(node, "Reference", string("ds")));
		authenticated_private.as_xml(cxml::add_child(node, "Reference", string("ds")));
	}

private:
	Reference authenticated_public;
	Reference authenticated_private;
};


class Signature
{
public:
	Signature () {}

	explicit Signature (shared_ptr<const cxml::Node> node)
		: signed_info (node->node_child ("SignedInfo"))
		, signature_value (node->string_child ("SignatureValue"))
	{
		for (auto i: node->node_child("KeyInfo")->node_children ("X509Data")) {
			x509_data.push_back(X509Data(i));
		}
	}

	void as_xml(xmlpp::Element* element) const
	{
		signed_info.as_xml(cxml::add_child(element, "SignedInfo", string("ds")));
		cxml::add_child(element, "SignatureValue", string("ds"))->add_child_text(signature_value);

		auto key_info_node = cxml::add_child(element, "KeyInfo", string("ds"));
		for (auto i: x509_data) {
			i.as_xml(cxml::add_child(key_info_node, "X509Data", string("ds")));
		}
	}

	SignedInfo signed_info;
	string signature_value;
	vector<X509Data> x509_data;
};


class AuthenticatedPrivate
{
public:
	AuthenticatedPrivate () {}

	explicit AuthenticatedPrivate (shared_ptr<const cxml::Node> node)
	{
		for (auto i: node->node_children ("EncryptedKey")) {
			encrypted_key.push_back (i->node_child("CipherData")->string_child("CipherValue"));
		}
	}

	void as_xml (xmlpp::Element* node, map<string, xmlpp::Attribute *>& references) const
	{
		references["ID_AuthenticatedPrivate"] = node->set_attribute ("Id", "ID_AuthenticatedPrivate");

		for (auto i: encrypted_key) {
			auto encrypted_key = cxml::add_child(node, "EncryptedKey", string("enc"));
			/* XXX: hack for testing with Dolby */
			encrypted_key->set_namespace_declaration ("http://www.w3.org/2001/04/xmlenc#", "enc");
			auto encryption_method = cxml::add_child(encrypted_key, "EncryptionMethod", string("enc"));
			encryption_method->set_attribute ("Algorithm", "http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p");
			auto digest_method = cxml::add_child(encryption_method, "DigestMethod", string("ds"));
			/* XXX: hack for testing with Dolby */
			digest_method->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "ds");
			digest_method->set_attribute ("Algorithm", "http://www.w3.org/2000/09/xmldsig#sha1");
			auto cipher_data = cxml::add_child(encrypted_key, "CipherData", string("enc"));
			cxml::add_child(cipher_data, "CipherValue", string("enc"))->add_child_text(i);
		}
	}

	vector<string> encrypted_key;
};


class TypedKeyId
{
public:
	TypedKeyId () {}

	explicit TypedKeyId (shared_ptr<const cxml::Node> node)
		: key_type (node->string_child ("KeyType"))
		, key_id (remove_urn_uuid (node->string_child ("KeyId")))
	{

	}

	TypedKeyId (string type, string id)
		: key_type (type)
		, key_id (id)
	{}

	void as_xml (xmlpp::Element* node) const
	{
		auto type = cxml::add_child(node, "KeyType");
		type->add_child_text (key_type);
		cxml::add_text_child(node, "KeyId", "urn:uuid:" + key_id);
		/* XXX: this feels like a bit of a hack */
		if (key_type == "MDEK") {
			type->set_attribute ("scope", "http://www.dolby.com/cp850/2012/KDM#kdm-key-type");
		} else {
			type->set_attribute ("scope", "http://www.smpte-ra.org/430-1/2006/KDM#kdm-key-type");
		}
	}

	string key_type;
	string key_id;
};


class KeyIdList
{
public:
	KeyIdList () {}

	explicit KeyIdList (shared_ptr<const cxml::Node> node)
	{
		for (auto i: node->node_children ("TypedKeyId")) {
			typed_key_id.push_back(TypedKeyId(i));
		}
	}

	void as_xml (xmlpp::Element* node) const
	{
		for (auto const& i: typed_key_id) {
			i.as_xml(cxml::add_child(node, ("TypedKeyId")));
		}
	}

	vector<TypedKeyId> typed_key_id;
};


class AuthorizedDeviceInfo
{
public:
	AuthorizedDeviceInfo () {}

	explicit AuthorizedDeviceInfo (shared_ptr<const cxml::Node> node)
		: device_list_identifier (remove_urn_uuid (node->string_child ("DeviceListIdentifier")))
		, device_list_description (node->optional_string_child ("DeviceListDescription"))
	{
		for (auto i: node->node_child("DeviceList")->node_children("CertificateThumbprint")) {
			certificate_thumbprints.push_back (i->content ());
		}
	}

	void as_xml (xmlpp::Element* node) const
	{
		cxml::add_text_child(node, "DeviceListIdentifier", "urn:uuid:" + device_list_identifier);
		if (device_list_description) {
			cxml::add_text_child(node, "DeviceListDescription", device_list_description.get());
		}
		auto device_list = cxml::add_child(node, "DeviceList");
		for (auto i: certificate_thumbprints) {
			cxml::add_text_child(device_list, "CertificateThumbprint", i);
		}
	}

	/** DeviceListIdentifier without the urn:uuid: prefix */
	string device_list_identifier;
	boost::optional<string> device_list_description;
	std::vector<string> certificate_thumbprints;
};


class X509IssuerSerial
{
public:
	X509IssuerSerial () {}

	explicit X509IssuerSerial (shared_ptr<const cxml::Node> node)
		: x509_issuer_name (node->string_child ("X509IssuerName"))
		, x509_serial_number (node->string_child ("X509SerialNumber"))
	{

	}

	void as_xml (xmlpp::Element* node) const
	{
		cxml::add_child(node, "X509IssuerName", string("ds"))->add_child_text(x509_issuer_name);
		cxml::add_child(node, "X509SerialNumber", string("ds"))->add_child_text(x509_serial_number);
	}

	string x509_issuer_name;
	string x509_serial_number;
};


class Recipient
{
public:
	Recipient () {}

	explicit Recipient (shared_ptr<const cxml::Node> node)
		: x509_issuer_serial (node->node_child ("X509IssuerSerial"))
		, x509_subject_name (node->string_child ("X509SubjectName"))
	{

	}

	void as_xml (xmlpp::Element* node) const
	{
		x509_issuer_serial.as_xml(cxml::add_child(node, "X509IssuerSerial"));
		cxml::add_text_child(node, "X509SubjectName", x509_subject_name);
	}

	X509IssuerSerial x509_issuer_serial;
	string x509_subject_name;
};


class KDMRequiredExtensions
{
public:
	KDMRequiredExtensions () {}

	explicit KDMRequiredExtensions (shared_ptr<const cxml::Node> node)
		: recipient (node->node_child ("Recipient"))
		, composition_playlist_id (remove_urn_uuid (node->string_child ("CompositionPlaylistId")))
		, content_title_text (node->string_child ("ContentTitleText"))
		, not_valid_before (node->string_child ("ContentKeysNotValidBefore"))
		, not_valid_after (node->string_child ("ContentKeysNotValidAfter"))
		, authorized_device_info (node->node_child ("AuthorizedDeviceInfo"))
		, key_id_list (node->node_child ("KeyIdList"))
	{
		disable_forensic_marking_picture = false;
		disable_forensic_marking_audio = optional<int>();
		if (node->optional_node_child("ForensicMarkFlagList")) {
			for (auto i: node->node_child("ForensicMarkFlagList")->node_children("ForensicMarkFlag")) {
				if (i->content() == picture_disable) {
					disable_forensic_marking_picture = true;
				} else if (starts_with(i->content(), audio_disable)) {
					disable_forensic_marking_audio = 0;
					string const above = audio_disable + "-above-channel-";
					if (starts_with(i->content(), above)) {
						auto above_number = i->content().substr(above.length());
						if (above_number == "") {
							throw KDMFormatError("Badly-formatted ForensicMarkFlag");
						}
						disable_forensic_marking_audio = atoi(above_number.c_str());
					}
				}
			}
		}
	}

	void as_xml (xmlpp::Element* node) const
	{
		node->set_attribute ("xmlns", "http://www.smpte-ra.org/schemas/430-1/2006/KDM");

		recipient.as_xml(cxml::add_child(node, "Recipient"));
		cxml::add_text_child(node, "CompositionPlaylistId", "urn:uuid:" + composition_playlist_id);
		cxml::add_text_child(node, "ContentTitleText", content_title_text);
		if (content_authenticator) {
			cxml::add_text_child(node, "ContentAuthenticator", content_authenticator.get());
		}
		cxml::add_text_child(node, "ContentKeysNotValidBefore", not_valid_before.as_string());
		cxml::add_text_child(node, "ContentKeysNotValidAfter", not_valid_after.as_string());
		if (authorized_device_info) {
			authorized_device_info->as_xml(cxml::add_child(node, "AuthorizedDeviceInfo"));
		}
		key_id_list.as_xml(cxml::add_child(node, "KeyIdList"));

		if (disable_forensic_marking_picture || disable_forensic_marking_audio) {
			auto forensic_mark_flag_list = cxml::add_child(node, "ForensicMarkFlagList");
			if (disable_forensic_marking_picture) {
				cxml::add_text_child(forensic_mark_flag_list, "ForensicMarkFlag", picture_disable);
			}
			if (disable_forensic_marking_audio) {
				auto mrkflg = audio_disable;
				if (*disable_forensic_marking_audio > 0) {
					mrkflg += String::compose ("-above-channel-%1", *disable_forensic_marking_audio);
				}
				cxml::add_text_child(forensic_mark_flag_list, "ForensicMarkFlag", mrkflg);
			}
		}
	}

	Recipient recipient;
	string composition_playlist_id;
	boost::optional<string> content_authenticator;
	string content_title_text;
	LocalTime not_valid_before;
	LocalTime not_valid_after;
	bool disable_forensic_marking_picture;
	optional<int> disable_forensic_marking_audio;
	boost::optional<AuthorizedDeviceInfo> authorized_device_info;
	KeyIdList key_id_list;

private:
	static const string picture_disable;
	static const string audio_disable;
};


const string KDMRequiredExtensions::picture_disable = "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-picture-disable";
const string KDMRequiredExtensions::audio_disable = "http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-audio-disable";


class RequiredExtensions
{
public:
	RequiredExtensions () {}

	explicit RequiredExtensions (shared_ptr<const cxml::Node> node)
		: kdm_required_extensions (node->node_child ("KDMRequiredExtensions"))
	{

	}

	void as_xml (xmlpp::Element* node) const
	{
		kdm_required_extensions.as_xml(cxml::add_child(node, "KDMRequiredExtensions"));
	}

	KDMRequiredExtensions kdm_required_extensions;
};


class AuthenticatedPublic
{
public:
	AuthenticatedPublic ()
		: message_id (make_uuid ())
		  /* XXX: hack for Dolby to see if there must be a not-empty annotation text */
		, annotation_text ("none")
		, issue_date (LocalTime().as_string ())
	{}

	explicit AuthenticatedPublic (shared_ptr<const cxml::Node> node)
		: message_id (remove_urn_uuid (node->string_child ("MessageId")))
		, annotation_text (node->optional_string_child ("AnnotationText"))
		, issue_date (node->string_child ("IssueDate"))
		, signer (node->node_child ("Signer"))
		, required_extensions (node->node_child ("RequiredExtensions"))
	{

	}

	void as_xml (xmlpp::Element* node, map<string, xmlpp::Attribute *>& references) const
	{
		references["ID_AuthenticatedPublic"] = node->set_attribute ("Id", "ID_AuthenticatedPublic");

		cxml::add_text_child(node, "MessageId", "urn:uuid:" + message_id);
		cxml::add_text_child(node, "MessageType", "http://www.smpte-ra.org/430-1/2006/KDM#kdm-key-type");
		if (annotation_text) {
			cxml::add_text_child(node, "AnnotationText", annotation_text.get());
		}
		cxml::add_text_child(node, "IssueDate", issue_date);

		signer.as_xml(cxml::add_child(node, "Signer"));
		required_extensions.as_xml(cxml::add_child(node, "RequiredExtensions"));

		cxml::add_child(node, "NonCriticalExtensions");
	}

	string message_id;
	optional<string> annotation_text;
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

	explicit EncryptedKDMData (shared_ptr<const cxml::Node> node)
		: authenticated_public (node->node_child ("AuthenticatedPublic"))
		, authenticated_private (node->node_child ("AuthenticatedPrivate"))
		, signature (node->node_child ("Signature"))
	{

	}

	shared_ptr<xmlpp::Document> as_xml () const
	{
		auto document = make_shared<xmlpp::Document>();
		auto root = document->create_root_node("DCinemaSecurityMessage", "http://www.smpte-ra.org/schemas/430-3/2006/ETM");
		root->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "ds");
		root->set_namespace_declaration ("http://www.w3.org/2001/04/xmlenc#", "enc");
		map<string, xmlpp::Attribute *> references;
		authenticated_public.as_xml(cxml::add_child(root, "AuthenticatedPublic"), references);
		authenticated_private.as_xml(cxml::add_child(root, "AuthenticatedPrivate"), references);
		signature.as_xml(cxml::add_child(root, "Signature", string("ds")));

		for (auto i: references) {
			xmlAddID (0, document->cobj(), (const xmlChar *) i.first.c_str(), i.second->cobj());
		}

		indent (document->get_root_node(), 0);
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
	try {
		auto doc = make_shared<cxml::Document>("DCinemaSecurityMessage");
		doc->read_string (s);
		_data = new data::EncryptedKDMData (doc);
	} catch (xmlpp::parse_error& e) {
		throw KDMFormatError (e.what ());
	} catch (xmlpp::internal_error& e) {
		throw KDMFormatError(e.what());
	} catch (cxml::Error& e) {
		throw KDMFormatError(e.what());
	}
}


EncryptedKDM::EncryptedKDM (
	shared_ptr<const CertificateChain> signer,
	Certificate recipient,
	vector<string> trusted_devices,
	string cpl_id,
	string content_title_text,
	optional<string> annotation_text,
	LocalTime not_valid_before,
	LocalTime not_valid_after,
	Formulation formulation,
	bool disable_forensic_marking_picture,
	optional<int> disable_forensic_marking_audio,
	vector<pair<string, string>> key_ids,
	vector<string> keys
	)
	: _data (new data::EncryptedKDMData)
{
	/* Fill our XML-ish description in with the juicy bits that the caller has given */

	/* Our ideas, based on http://isdcf.com/papers/ISDCF-Doc5-kdm-certs.pdf, about the KDM types are:
	 *
	 * Type                               Trusted-device thumb  ContentAuthenticator
	 * MODIFIED_TRANSITIONAL_1            assume-trust          No
	 * MULTIPLE_MODIFIED_TRANSITIONAL_1   as specified          No
	 * DCI_ANY                            assume-trust          Yes
	 * DCI_SPECIFIC                       as specified          Yes
	 */

	auto& aup = _data->authenticated_public;
	aup.signer.x509_issuer_name = signer->leaf().issuer ();
	aup.signer.x509_serial_number = signer->leaf().serial ();
	aup.annotation_text = annotation_text;

	auto& kre = _data->authenticated_public.required_extensions.kdm_required_extensions;
	kre.recipient.x509_issuer_serial.x509_issuer_name = recipient.issuer ();
	kre.recipient.x509_issuer_serial.x509_serial_number = recipient.serial ();
	kre.recipient.x509_subject_name = recipient.subject ();
	kre.composition_playlist_id = cpl_id;
	if (formulation == Formulation::DCI_ANY || formulation == Formulation::DCI_SPECIFIC) {
		kre.content_authenticator = signer->leaf().thumbprint ();
	}
	kre.content_title_text = content_title_text;
	kre.not_valid_before = not_valid_before;
	kre.not_valid_after = not_valid_after;
	kre.disable_forensic_marking_picture = disable_forensic_marking_picture;
	kre.disable_forensic_marking_audio = disable_forensic_marking_audio;

	kre.authorized_device_info = data::AuthorizedDeviceInfo ();
	kre.authorized_device_info->device_list_identifier = make_uuid ();
	auto n = recipient.subject_common_name ();
	if (n.find (".") != string::npos) {
		n = n.substr (n.find (".") + 1);
	}
	kre.authorized_device_info->device_list_description = n;

	if (formulation == Formulation::MODIFIED_TRANSITIONAL_1 || formulation == Formulation::DCI_ANY) {
		/* Use the "assume trust" thumbprint */
		kre.authorized_device_info->certificate_thumbprints.push_back ("2jmj7l5rSw0yVb/vlWAYkK/YBwk=");
	} else if (formulation == Formulation::MULTIPLE_MODIFIED_TRANSITIONAL_1 || formulation == Formulation::DCI_SPECIFIC) {
		if (trusted_devices.empty ()) {
			/* Fall back on the "assume trust" thumbprint so we
			   can generate "modified-transitional-1" KDMs
			   together with "multiple-modified-transitional-1"
			   KDMs in one go, and similarly for "dci-any" etc.
			*/
			kre.authorized_device_info->certificate_thumbprints.push_back ("2jmj7l5rSw0yVb/vlWAYkK/YBwk=");
		} else {
			/* As I read the standard we should use the
			   recipient /and/ other trusted device thumbprints
			   here. MJD reports that this doesn't work with
			   his setup; a working KDM does not include the
			   recipient's thumbprint (recipient.thumbprint()).
			   Waimea uses only the trusted devices here, too.
			*/
			for (auto i: trusted_devices) {
				kre.authorized_device_info->certificate_thumbprints.push_back(i);
			}
		}
	}

	for (auto i: key_ids) {
		kre.key_id_list.typed_key_id.push_back(data::TypedKeyId(i.first, i.second));
	}

	_data->authenticated_private.encrypted_key = keys;

	/* Read the XML so far and sign it */
	auto doc = _data->as_xml ();
	for (auto i: doc->get_root_node()->get_children()) {
		if (i->get_name() == "Signature") {
			signer->add_signature_value(dynamic_cast<xmlpp::Element*>(i), "ds", false);
		}
	}

	/* Read the bits that add_signature_value did back into our variables */
	auto signed_doc = make_shared<cxml::Node>(doc->get_root_node());
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
	File f(path, "w");
	if (!f) {
		throw FileError ("Could not open KDM file for writing", path, errno);
	}
	auto const x = as_xml ();
	if (f.write(x.c_str(), 1, x.length()) != x.length()) {
		throw FileError ("Could not write to KDM file", path, errno);
	}
}


string
EncryptedKDM::as_xml () const
{
	return _data->as_xml()->write_to_string ("UTF-8");
}


vector<string>
EncryptedKDM::keys () const
{
	return _data->authenticated_private.encrypted_key;
}


string
EncryptedKDM::id () const
{
	return _data->authenticated_public.message_id;
}


optional<string>
EncryptedKDM::annotation_text () const
{
	return _data->authenticated_public.annotation_text;
}


string
EncryptedKDM::content_title_text () const
{
	return _data->authenticated_public.required_extensions.kdm_required_extensions.content_title_text;
}


string
EncryptedKDM::cpl_id () const
{
	return _data->authenticated_public.required_extensions.kdm_required_extensions.composition_playlist_id;
}


string
EncryptedKDM::issue_date () const
{
	return _data->authenticated_public.issue_date;
}


LocalTime
EncryptedKDM::not_valid_before () const
{
	return _data->authenticated_public.required_extensions.kdm_required_extensions.not_valid_before;
}


LocalTime
EncryptedKDM::not_valid_after () const
{
	return _data->authenticated_public.required_extensions.kdm_required_extensions.not_valid_after;
}


string
EncryptedKDM::recipient_x509_subject_name () const
{
	return _data->authenticated_public.required_extensions.kdm_required_extensions.recipient.x509_subject_name;
}


CertificateChain
EncryptedKDM::signer_certificate_chain () const
{
	CertificateChain chain;
	for (auto const& i: _data->signature.x509_data) {
		string s = "-----BEGIN CERTIFICATE-----\n" + i.x509_certificate + "\n-----END CERTIFICATE-----";
		chain.add (Certificate(s));
	}
	return chain;
}


bool
dcp::operator== (EncryptedKDM const & a, EncryptedKDM const & b)
{
	/* Not exactly efficient... */
	return a.as_xml() == b.as_xml();
}
