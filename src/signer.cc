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

/** @file  src/signer.cc
 *  @brief Signer class.
 */

#include "signer.h"
#include "exceptions.h"
#include "certificate_chain.h"
#include "util.h"
#include <libcxml/cxml.h>
#include <libxml++/libxml++.h>
#include <xmlsec/xmldsig.h>
#include <xmlsec/dl.h>
#include <xmlsec/app.h>
#include <xmlsec/crypto.h>
#include <openssl/pem.h>
#include "compose.hpp"

using std::string;
using std::list;
using std::cout;
using boost::shared_ptr;
using namespace dcp;

Signer::Signer (boost::filesystem::path openssl)
{
	boost::filesystem::path directory = make_certificate_chain (openssl);

	_certificates.add (shared_ptr<dcp::Certificate> (new dcp::Certificate (dcp::file_to_string (directory / "ca.self-signed.pem"))));
	_certificates.add (shared_ptr<dcp::Certificate> (new dcp::Certificate (dcp::file_to_string (directory / "intermediate.signed.pem"))));
	_certificates.add (shared_ptr<dcp::Certificate> (new dcp::Certificate (dcp::file_to_string (directory / "leaf.signed.pem"))));

	_key = dcp::file_to_string (directory / "leaf.key");

	boost::filesystem::remove_all (directory);
}
	

/** Add a &lt;Signer&gt; and &lt;ds:Signature&gt; nodes to an XML node.
 *  @param parent XML node to add to.
 *  @param standard INTEROP or SMPTE.
 */
void
Signer::sign (xmlpp::Element* parent, Standard standard) const
{
	/* <Signer> */
	
	xmlpp::Element* signer = parent->add_child("Signer");
	xmlpp::Element* data = signer->add_child("X509Data", "dsig");
	xmlpp::Element* serial_element = data->add_child("X509IssuerSerial", "dsig");
	serial_element->add_child("X509IssuerName", "dsig")->add_child_text (_certificates.leaf()->issuer());
	serial_element->add_child("X509SerialNumber", "dsig")->add_child_text (_certificates.leaf()->serial());
	data->add_child("X509SubjectName", "dsig")->add_child_text (_certificates.leaf()->subject());

	/* <Signature> */
	
	xmlpp::Element* signature = parent->add_child("Signature", "dsig");
	
	xmlpp::Element* signed_info = signature->add_child ("SignedInfo", "dsig");
	signed_info->add_child("CanonicalizationMethod", "dsig")->set_attribute ("Algorithm", "http://www.w3.org/TR/2001/REC-xml-c14n-20010315");
	
	if (standard == INTEROP) {
		signed_info->add_child("SignatureMethod", "dsig")->set_attribute("Algorithm", "http://www.w3.org/2000/09/xmldsig#rsa-sha1");
	} else {
		signed_info->add_child("SignatureMethod", "dsig")->set_attribute("Algorithm", "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256");
	}
	
	xmlpp::Element* reference = signed_info->add_child("Reference", "dsig");
	reference->set_attribute ("URI", "");

	xmlpp::Element* transforms = reference->add_child("Transforms", "dsig");
	transforms->add_child("Transform", "dsig")->set_attribute (
		"Algorithm", "http://www.w3.org/2000/09/xmldsig#enveloped-signature"
		);

	reference->add_child("DigestMethod", "dsig")->set_attribute("Algorithm", "http://www.w3.org/2000/09/xmldsig#sha1");
	/* This will be filled in by the signing later */
	reference->add_child("DigestValue", "dsig");

	signature->add_child("SignatureValue", "dsig");
	signature->add_child("KeyInfo", "dsig");
	add_signature_value (signature, "dsig");
}


/** Sign an XML node.
 *
 *  @param parent Node to sign.
 *  @param ns Namespace to use for the signature XML nodes.
 */
void
Signer::add_signature_value (xmlpp::Node* parent, string ns) const
{
	cxml::Node cp (parent);
	xmlpp::Node* key_info = cp.node_child("KeyInfo")->node ();

	/* Add the certificate chain to the KeyInfo child node of parent */
	CertificateChain::List c = _certificates.leaf_to_root ();
	for (CertificateChain::List::iterator i = c.begin(); i != c.end(); ++i) {
		xmlpp::Element* data = key_info->add_child("X509Data", ns);
		
		{
			xmlpp::Element* serial = data->add_child("X509IssuerSerial", ns);
			serial->add_child("X509IssuerName", ns)->add_child_text((*i)->issuer ());
			serial->add_child("X509SerialNumber", ns)->add_child_text((*i)->serial ());
		}
		
		data->add_child("X509Certificate", ns)->add_child_text((*i)->certificate());
	}

	xmlSecDSigCtxPtr signature_context = xmlSecDSigCtxCreate (0);
	if (signature_context == 0) {
		throw MiscError ("could not create signature context");
	}

	signature_context->signKey = xmlSecCryptoAppKeyLoadMemory (
		reinterpret_cast<const unsigned char *> (_key.c_str()), _key.size(), xmlSecKeyDataFormatPem, 0, 0, 0
		);
	
	if (signature_context->signKey == 0) {
		throw FileError ("could not load private key file", _key, 0);
	}

	/* XXX: set key name to the file name: is this right? */
	if (xmlSecKeySetName (signature_context->signKey, reinterpret_cast<const xmlChar *> (_key.c_str())) < 0) {
		throw MiscError ("could not set key name");
	}

	int const r = xmlSecDSigCtxSign (signature_context, parent->cobj ());
	if (r < 0) {
		throw MiscError (String::compose ("could not sign (%1)", r));
	}

	xmlSecDSigCtxDestroy (signature_context);
}

bool
Signer::valid () const
{
	if (!_certificates.valid ()) {
		return false;
	}

	BIO* bio = BIO_new_mem_buf (const_cast<char *> (_key.c_str ()), -1);
	if (!bio) {
		throw MiscError ("could not create memory BIO");
	}
	
	RSA* private_key = PEM_read_bio_RSAPrivateKey (bio, 0, 0, 0);
	RSA* public_key = _certificates.leaf()->public_key ();
	bool const valid = !BN_cmp (private_key->n, public_key->n);
	BIO_free (bio);

	return valid;
}
