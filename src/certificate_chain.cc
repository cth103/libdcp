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


/** @file  src/certificate_chain.cc
 *  @brief CertificateChain class
 */


#include "certificate_chain.h"
#include "compose.hpp"
#include "dcp_assert.h"
#include "exceptions.h"
#include "util.h"
#include "warnings.h"
#include <asdcp/KM_util.h>
#include <libcxml/cxml.h>
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <xmlsec/xmldsig.h>
#include <xmlsec/dl.h>
#include <xmlsec/app.h>
#include <xmlsec/crypto.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <iostream>


using std::string;
using std::ofstream;
using std::ifstream;
using std::runtime_error;
using namespace dcp;


/** Run a shell command.
 *  @param cmd Command to run (UTF8-encoded).
 */
static void
command (string cmd)
{
#ifdef LIBDCP_WINDOWS
	/* We need to use CreateProcessW on Windows so that the UTF-8/16 mess
	   is handled correctly.
	*/
	int const wn = MultiByteToWideChar (CP_UTF8, 0, cmd.c_str(), -1, 0, 0);
	auto buffer = new wchar_t[wn];
	if (MultiByteToWideChar (CP_UTF8, 0, cmd.c_str(), -1, buffer, wn) == 0) {
		delete[] buffer;
		return;
	}

	int code = 1;

	STARTUPINFOW startup_info;
	memset (&startup_info, 0, sizeof (startup_info));
	startup_info.cb = sizeof (startup_info);
	PROCESS_INFORMATION process_info;

	/* XXX: this doesn't actually seem to work; failing commands end up with
	   a return code of 0
	*/
	if (CreateProcessW (0, buffer, 0, 0, FALSE, CREATE_NO_WINDOW, 0, 0, &startup_info, &process_info)) {
		WaitForSingleObject (process_info.hProcess, INFINITE);
		DWORD c;
		if (GetExitCodeProcess (process_info.hProcess, &c)) {
			code = c;
		}
		CloseHandle (process_info.hProcess);
		CloseHandle (process_info.hThread);
	}

	delete[] buffer;
#else
	cmd += " 2> /dev/null";
	int const r = system (cmd.c_str ());
	int const code = WEXITSTATUS (r);
#endif
	if (code) {
		throw dcp::MiscError (String::compose ("error %1 in %2 within %3", code, cmd, boost::filesystem::current_path().string()));
	}
}


/** Extract a public key from a private key and create a SHA1 digest of it.
 *  @param private_key Private key
 *  @param openssl openssl binary name (or full path if openssl is not on the system path).
 *  @return SHA1 digest of corresponding public key, with escaped / characters.
 */
static string
public_key_digest (boost::filesystem::path private_key, boost::filesystem::path openssl)
{
	boost::filesystem::path public_name = private_key.string() + ".public";

	/* Create the public key from the private key */
	command (String::compose("\"%1\" rsa -outform PEM -pubout -in %2 -out %3", openssl.string(), private_key.string(), public_name.string()));

	/* Read in the public key from the file */

	string pub;
	ifstream f (public_name.string().c_str());
	if (!f.good ()) {
		throw dcp::MiscError ("public key not found");
	}

	bool read = false;
	while (f.good ()) {
		string line;
		getline (f, line);
		if (line.length() >= 10 && line.substr(0, 10) == "-----BEGIN") {
			read = true;
		} else if (line.length() >= 8 && line.substr(0, 8) == "-----END") {
			break;
		} else if (read) {
			pub += line;
		}
	}

	/* Decode the base64 of the public key */

	unsigned char buffer[512];
	int const N = dcp::base64_decode (pub, buffer, 1024);

	/* Hash it with SHA1 (without the first 24 bytes, for reasons that are not entirely clear) */

	SHA_CTX context;
	if (!SHA1_Init (&context)) {
		throw dcp::MiscError ("could not init SHA1 context");
	}

	if (!SHA1_Update (&context, buffer + 24, N - 24)) {
		throw dcp::MiscError ("could not update SHA1 digest");
	}

	unsigned char digest[SHA_DIGEST_LENGTH];
	if (!SHA1_Final (digest, &context)) {
		throw dcp::MiscError ("could not finish SHA1 digest");
	}

	char digest_base64[64];
	string dig = Kumu::base64encode (digest, SHA_DIGEST_LENGTH, digest_base64, 64);
#ifdef LIBDCP_WINDOWS
	boost::replace_all (dig, "/", "\\/");
#else
	boost::replace_all (dig, "/", "\\\\/");
#endif
	return dig;
}


CertificateChain::CertificateChain (
	boost::filesystem::path openssl,
	string organisation,
	string organisational_unit,
	string root_common_name,
	string intermediate_common_name,
	string leaf_common_name
	)
{
	/* Valid for 40 years */
	int const days = 365 * 40;

	auto directory = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path ();
	boost::filesystem::create_directories (directory);

	auto const cwd = boost::filesystem::current_path ();
	boost::filesystem::current_path (directory);

	string quoted_openssl = "\"" + openssl.string() + "\"";

	command (quoted_openssl + " genrsa -out ca.key 2048");

	{
		ofstream f ("ca.cnf");
		f << "[ req ]\n"
		  << "distinguished_name = req_distinguished_name\n"
		  << "x509_extensions	= v3_ca\n"
		  << "string_mask = nombstr\n"
		  << "[ v3_ca ]\n"
		  << "basicConstraints = critical,CA:true,pathlen:3\n"
		  << "keyUsage = keyCertSign,cRLSign\n"
		  << "subjectKeyIdentifier = hash\n"
		  << "authorityKeyIdentifier = keyid:always,issuer:always\n"
		  << "[ req_distinguished_name ]\n"
		  << "O = Unique organization name\n"
		  << "OU = Organization unit\n"
		  << "CN = Entity and dnQualifier\n";
	}

	string const ca_subject = "/O=" + organisation +
		"/OU=" + organisational_unit +
		"/CN=" + root_common_name +
		"/dnQualifier=" + public_key_digest ("ca.key", openssl);

	{
		command (
			String::compose (
				"%1 req -new -x509 -sha256 -config ca.cnf -days %2 -set_serial 5"
				" -subj \"%3\" -key ca.key -outform PEM -out ca.self-signed.pem",
				quoted_openssl, days, ca_subject
				)
			);
	}

	command (quoted_openssl + " genrsa -out intermediate.key 2048");

	{
		ofstream f ("intermediate.cnf");
		f << "[ default ]\n"
		  << "distinguished_name = req_distinguished_name\n"
		  << "x509_extensions = v3_ca\n"
		  << "string_mask = nombstr\n"
		  << "[ v3_ca ]\n"
		  << "basicConstraints = critical,CA:true,pathlen:2\n"
		  << "keyUsage = keyCertSign,cRLSign\n"
		  << "subjectKeyIdentifier = hash\n"
		  << "authorityKeyIdentifier = keyid:always,issuer:always\n"
		  << "[ req_distinguished_name ]\n"
		  << "O = Unique organization name\n"
		  << "OU = Organization unit\n"
		  << "CN = Entity and dnQualifier\n";
	}

	string const inter_subject = "/O=" + organisation +
		"/OU=" + organisational_unit +
		"/CN=" + intermediate_common_name +
		"/dnQualifier="	+ public_key_digest ("intermediate.key", openssl);

	{
		command (
			String::compose (
				"%1 req -new -config intermediate.cnf -days %2 -subj \"%3\" -key intermediate.key -out intermediate.csr",
				quoted_openssl, days - 1, inter_subject
				)
			);
	}

	command (
		String::compose (
			"%1 x509 -req -sha256 -days %2 -CA ca.self-signed.pem -CAkey ca.key -set_serial 6"
			" -in intermediate.csr -extfile intermediate.cnf -extensions v3_ca -out intermediate.signed.pem",
			quoted_openssl, days - 1
			)
		);

	command (quoted_openssl + " genrsa -out leaf.key 2048");

	{
		ofstream f ("leaf.cnf");
		f << "[ default ]\n"
		  << "distinguished_name = req_distinguished_name\n"
		  << "x509_extensions	= v3_ca\n"
		  << "string_mask = nombstr\n"
		  << "[ v3_ca ]\n"
		  << "basicConstraints = critical,CA:false\n"
		  << "keyUsage = digitalSignature,keyEncipherment\n"
		  << "subjectKeyIdentifier = hash\n"
		  << "authorityKeyIdentifier = keyid,issuer:always\n"
		  << "[ req_distinguished_name ]\n"
		  << "O = Unique organization name\n"
		  << "OU = Organization unit\n"
		  << "CN = Entity and dnQualifier\n";
	}

	string const leaf_subject = "/O=" + organisation +
		"/OU=" + organisational_unit +
		"/CN=" + leaf_common_name +
		"/dnQualifier="	+ public_key_digest ("leaf.key", openssl);

	{
		command (
			String::compose (
				"%1 req -new -config leaf.cnf -days %2 -subj \"%3\" -key leaf.key -outform PEM -out leaf.csr",
				quoted_openssl, days - 2, leaf_subject
				)
			);
	}

	command (
		String::compose (
			"%1 x509 -req -sha256 -days %2 -CA intermediate.signed.pem -CAkey intermediate.key"
			" -set_serial 7 -in leaf.csr -extfile leaf.cnf -extensions v3_ca -out leaf.signed.pem",
			quoted_openssl, days - 2
			)
		);

	boost::filesystem::current_path (cwd);

	_certificates.push_back (dcp::Certificate(dcp::file_to_string(directory / "ca.self-signed.pem")));
	_certificates.push_back (dcp::Certificate(dcp::file_to_string(directory / "intermediate.signed.pem")));
	_certificates.push_back (dcp::Certificate(dcp::file_to_string(directory / "leaf.signed.pem")));

	_key = dcp::file_to_string (directory / "leaf.key");

	boost::filesystem::remove_all (directory);
}


CertificateChain::CertificateChain (string s)
{
	while (true) {
		try {
			Certificate c;
			s = c.read_string (s);
			_certificates.push_back (c);
		} catch (MiscError& e) {
			/* Failed to read a certificate, just stop */
			break;
		}
	}

	/* This will throw an exception if the chain cannot be ordered */
	leaf_to_root ();
}


Certificate
CertificateChain::root () const
{
	DCP_ASSERT (!_certificates.empty());
	return root_to_leaf().front();
}


Certificate
CertificateChain::leaf () const
{
	DCP_ASSERT (!_certificates.empty());
	return root_to_leaf().back();
}


CertificateChain::List
CertificateChain::leaf_to_root () const
{
	auto l = root_to_leaf ();
	std::reverse (l.begin(), l.end());
	return l;
}


CertificateChain::List
CertificateChain::unordered () const
{
	return _certificates;
}


void
CertificateChain::add (Certificate c)
{
	_certificates.push_back (c);
}


void
CertificateChain::remove (Certificate c)
{
	auto i = std::find(_certificates.begin(), _certificates.end(), c);
	if (i != _certificates.end()) {
		_certificates.erase (i);
	}
}


void
CertificateChain::remove (int i)
{
	auto j = _certificates.begin ();
        while (j != _certificates.end () && i > 0) {
		--i;
		++j;
	}

	if (j != _certificates.end ()) {
		_certificates.erase (j);
	}
}


bool
CertificateChain::chain_valid () const
{
	return chain_valid (_certificates);
}


bool
CertificateChain::chain_valid (List const & chain) const
{
        /* Here I am taking a chain of certificates A/B/C/D and checking validity of B wrt A,
	   C wrt B and D wrt C.  It also appears necessary to check the issuer of B/C/D matches
	   the subject of A/B/C; I don't understand why.  I'm sure there's a better way of doing
	   this with OpenSSL but the documentation does not appear not likely to reveal it
	   any time soon.
	*/

	auto store = X509_STORE_new ();
	if (!store) {
		throw MiscError ("could not create X509 store");
	}

	/* Put all the certificates into the store */
	for (auto const& i: chain) {
		if (!X509_STORE_add_cert(store, i.x509())) {
			X509_STORE_free(store);
			return false;
		}
	}

	/* Verify each one */
	for (auto i = chain.begin(); i != chain.end(); ++i) {

		auto j = i;
		++j;
		if (j == chain.end ()) {
			break;
		}

		auto ctx = X509_STORE_CTX_new ();
		if (!ctx) {
			X509_STORE_free (store);
			throw MiscError ("could not create X509 store context");
		}

		X509_STORE_set_flags (store, 0);
		if (!X509_STORE_CTX_init (ctx, store, j->x509(), 0)) {
			X509_STORE_CTX_free (ctx);
			X509_STORE_free (store);
			throw MiscError ("could not initialise X509 store context");
		}

		int const v = X509_verify_cert (ctx);
		X509_STORE_CTX_free (ctx);

		if (v != 1) {
			X509_STORE_free (store);
			return false;
		}

		/* I don't know why OpenSSL doesn't check this stuff
		   in verify_cert, but without these checks the
		   certificates_validation8 test fails.
		*/
		if (j->issuer() != i->subject() || j->subject() == i->subject()) {
			X509_STORE_free (store);
			return false;
		}

	}

	X509_STORE_free (store);

	return true;
}


bool
CertificateChain::private_key_valid () const
{
	if (_certificates.empty ()) {
		return true;
	}

	if (!_key) {
		return false;
	}

	auto bio = BIO_new_mem_buf (const_cast<char *> (_key->c_str ()), -1);
	if (!bio) {
		throw MiscError ("could not create memory BIO");
	}

	auto private_key = PEM_read_bio_RSAPrivateKey (bio, 0, 0, 0);
	if (!private_key) {
		return false;
	}

	auto public_key = leaf().public_key ();

#if OPENSSL_VERSION_NUMBER > 0x10100000L
	BIGNUM const * private_key_n;
	RSA_get0_key(private_key, &private_key_n, 0, 0);
	BIGNUM const * public_key_n;
	RSA_get0_key(public_key, &public_key_n, 0, 0);
	if (!private_key_n || !public_key_n) {
		return false;
	}
	bool const valid = !BN_cmp (private_key_n, public_key_n);
#else
	bool const valid = !BN_cmp (private_key->n, public_key->n);
#endif
	BIO_free (bio);

	return valid;
}


bool
CertificateChain::valid (string* reason) const
{
	try {
		root_to_leaf ();
	} catch (CertificateChainError& e) {
		if (reason) {
			*reason = "certificates do not form a chain";
		}
		return false;
	}

	if (!private_key_valid ()) {
		if (reason) {
			*reason = "private key does not exist, or does not match leaf certificate";
		}
		return false;
	}

	return true;
}


CertificateChain::List
CertificateChain::root_to_leaf () const
{
	auto rtl = _certificates;
	std::sort (rtl.begin(), rtl.end());
	do {
		if (chain_valid (rtl)) {
			return rtl;
		}
	} while (std::next_permutation (rtl.begin(), rtl.end()));

	throw CertificateChainError ("certificate chain is not consistent");
}


void
CertificateChain::sign (xmlpp::Element* parent, Standard standard) const
{
	/* <Signer> */

	parent->add_child_text("  ");
	auto signer = parent->add_child("Signer");
	signer->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "dsig");
	auto data = signer->add_child("X509Data", "dsig");
	auto serial_element = data->add_child("X509IssuerSerial", "dsig");
	serial_element->add_child("X509IssuerName", "dsig")->add_child_text (leaf().issuer());
	serial_element->add_child("X509SerialNumber", "dsig")->add_child_text (leaf().serial());
	data->add_child("X509SubjectName", "dsig")->add_child_text (leaf().subject());

	indent (signer, 2);

	/* <Signature> */

	parent->add_child_text("\n  ");
	auto signature = parent->add_child("Signature");
	signature->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "dsig");
	signature->set_namespace ("dsig");
	parent->add_child_text("\n");

	auto signed_info = signature->add_child ("SignedInfo", "dsig");
	signed_info->add_child("CanonicalizationMethod", "dsig")->set_attribute ("Algorithm", "http://www.w3.org/TR/2001/REC-xml-c14n-20010315");

	if (standard == Standard::INTEROP) {
		signed_info->add_child("SignatureMethod", "dsig")->set_attribute("Algorithm", "http://www.w3.org/2000/09/xmldsig#rsa-sha1");
	} else {
		signed_info->add_child("SignatureMethod", "dsig")->set_attribute("Algorithm", "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256");
	}

	auto reference = signed_info->add_child("Reference", "dsig");
	reference->set_attribute ("URI", "");

	auto transforms = reference->add_child("Transforms", "dsig");
	transforms->add_child("Transform", "dsig")->set_attribute (
		"Algorithm", "http://www.w3.org/2000/09/xmldsig#enveloped-signature"
		);

	reference->add_child("DigestMethod", "dsig")->set_attribute("Algorithm", "http://www.w3.org/2000/09/xmldsig#sha1");
	/* This will be filled in by the signing later */
	reference->add_child("DigestValue", "dsig");

	signature->add_child("SignatureValue", "dsig");
	signature->add_child("KeyInfo", "dsig");
	add_signature_value (signature, "dsig", true);
}


void
CertificateChain::add_signature_value (xmlpp::Element* parent, string ns, bool add_indentation) const
{
	cxml::Node cp (parent);
	auto key_info = cp.node_child("KeyInfo")->node();

	/* Add the certificate chain to the KeyInfo child node of parent */
	for (auto const& i: leaf_to_root()) {
		auto data = key_info->add_child("X509Data", ns);

		{
			auto serial = data->add_child("X509IssuerSerial", ns);
			serial->add_child("X509IssuerName", ns)->add_child_text (i.issuer ());
			serial->add_child("X509SerialNumber", ns)->add_child_text (i.serial ());
		}

		data->add_child("X509Certificate", ns)->add_child_text (i.certificate());
	}

	auto signature_context = xmlSecDSigCtxCreate (0);
	if (signature_context == 0) {
		throw MiscError ("could not create signature context");
	}

	signature_context->signKey = xmlSecCryptoAppKeyLoadMemory (
		reinterpret_cast<const unsigned char *> (_key->c_str()), _key->size(), xmlSecKeyDataFormatPem, 0, 0, 0
		);

	if (signature_context->signKey == 0) {
		throw runtime_error ("could not read private key");
	}

	if (add_indentation) {
		indent (parent, 2);
	}
	int const r = xmlSecDSigCtxSign (signature_context, parent->cobj ());
	if (r < 0) {
		throw MiscError (String::compose ("could not sign (%1)", r));
	}

	xmlSecDSigCtxDestroy (signature_context);
}


string
CertificateChain::chain () const
{
	string o;
	for (auto const& i: root_to_leaf()) {
		o += i.certificate(true);
	}

	return o;
}
