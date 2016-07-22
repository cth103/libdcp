/*
    Copyright (C) 2013-2015 Carl Hetherington <cth@carlh.net>

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

/** @file  src/signer_chain.cc
 *  @brief Functions to make signer chains.
 */

#include "certificate_chain.h"
#include "exceptions.h"
#include "util.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include <asdcp/KM_util.h>
#include <libcxml/cxml.h>
#include <libxml++/libxml++.h>
#include <xmlsec/xmldsig.h>
#include <xmlsec/dl.h>
#include <xmlsec/app.h>
#include <xmlsec/crypto.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <fstream>

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
	wchar_t* buffer = new wchar_t[wn];
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
		locked_stringstream s;
		s << "error " << code << " in " << cmd << " within " << boost::filesystem::current_path();
		throw dcp::MiscError (s.str());
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
	locked_stringstream s;
	s << "\"" << openssl.string() << "\" rsa -outform PEM -pubout -in " << private_key.string() << " -out " << public_name.string ();
	command (s.str().c_str ());

	/* Read in the public key from the file */

	string pub;
	ifstream f (public_name.string().c_str ());
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
	boost::filesystem::path directory = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path ();
	boost::filesystem::create_directories (directory);

	boost::filesystem::path const cwd = boost::filesystem::current_path ();
	boost::filesystem::current_path (directory);

	string quoted_openssl = "\"" + openssl.string() + "\"";

	command (quoted_openssl + " genrsa -out ca.key 2048");

	{
		ofstream f ("ca.cnf");
		f << "[ req ]\n"
		  << "distinguished_name = req_distinguished_name\n"
		  << "x509_extensions	= v3_ca\n"
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
		locked_stringstream c;
		c << quoted_openssl
		  << " req -new -x509 -sha256 -config ca.cnf -days 3650 -set_serial 5"
		  << " -subj \"" << ca_subject << "\" -key ca.key -outform PEM -out ca.self-signed.pem";
		command (c.str().c_str());
	}

	command (quoted_openssl + " genrsa -out intermediate.key 2048");

	{
		ofstream f ("intermediate.cnf");
		f << "[ default ]\n"
		  << "distinguished_name = req_distinguished_name\n"
		  << "x509_extensions = v3_ca\n"
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
		locked_stringstream s;
		s << quoted_openssl
		  << " req -new -config intermediate.cnf -days 3649 -subj \"" << inter_subject << "\" -key intermediate.key -out intermediate.csr";
		command (s.str().c_str());
	}


	command (
		quoted_openssl +
		" x509 -req -sha256 -days 3649 -CA ca.self-signed.pem -CAkey ca.key -set_serial 6"
		" -in intermediate.csr -extfile intermediate.cnf -extensions v3_ca -out intermediate.signed.pem"
		);

	command (quoted_openssl + " genrsa -out leaf.key 2048");

	{
		ofstream f ("leaf.cnf");
		f << "[ default ]\n"
		  << "distinguished_name = req_distinguished_name\n"
		  << "x509_extensions	= v3_ca\n"
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
		locked_stringstream s;
		s << quoted_openssl << " req -new -config leaf.cnf -days 3648 -subj \"" << leaf_subject << "\" -key leaf.key -outform PEM -out leaf.csr";
		command (s.str().c_str());
	}

	command (
		quoted_openssl +
		" x509 -req -sha256 -days 3648 -CA intermediate.signed.pem -CAkey intermediate.key"
		" -set_serial 7 -in leaf.csr -extfile leaf.cnf -extensions v3_ca -out leaf.signed.pem"
		);

	boost::filesystem::current_path (cwd);

	_certificates.push_back (dcp::Certificate (dcp::file_to_string (directory / "ca.self-signed.pem")));
	_certificates.push_back (dcp::Certificate (dcp::file_to_string (directory / "intermediate.signed.pem")));
	_certificates.push_back (dcp::Certificate (dcp::file_to_string (directory / "leaf.signed.pem")));

	_key = dcp::file_to_string (directory / "leaf.key");

	boost::filesystem::remove_all (directory);
}

/** @return Root certificate */
Certificate
CertificateChain::root () const
{
	DCP_ASSERT (!_certificates.empty());
	return _certificates.front ();
}

/** @return Leaf certificate */
Certificate
CertificateChain::leaf () const
{
	DCP_ASSERT (_certificates.size() >= 2);
	return _certificates.back ();
}

/** @return Certificates in order from root to leaf */
CertificateChain::List
CertificateChain::root_to_leaf () const
{
	return _certificates;
}

/** @return Certificates in order from leaf to root */
CertificateChain::List
CertificateChain::leaf_to_root () const
{
	List c = _certificates;
	c.reverse ();
	return c;
}

/** Add a certificate to the end of the chain.
 *  @param c Certificate to add.
 */
void
CertificateChain::add (Certificate c)
{
	_certificates.push_back (c);
}

/** Remove a certificate from the chain.
 *  @param c Certificate to remove.
 */
void
CertificateChain::remove (Certificate c)
{
	_certificates.remove (c);
}

/** Remove the i'th certificate in the list, as listed
 *  from root to leaf.
 */
void
CertificateChain::remove (int i)
{
	List::iterator j = _certificates.begin ();
        while (j != _certificates.end () && i > 0) {
		--i;
		++j;
	}

	if (j != _certificates.end ()) {
		_certificates.erase (j);
	}
}

/** Check to see if the chain is valid (i.e. root signs the intermediate, intermediate
 *  signs the leaf and so on) and that the private key (if there is one) matches the
 *  leaf certificate.
 *  @return true if it's ok, false if not.
 */
bool
CertificateChain::valid () const
{
	/* Check the certificate chain */

	X509_STORE* store = X509_STORE_new ();
	if (!store) {
		return false;
	}

	for (List::const_iterator i = _certificates.begin(); i != _certificates.end(); ++i) {

		List::const_iterator j = i;
		++j;
		if (j ==  _certificates.end ()) {
			break;
		}

		if (!X509_STORE_add_cert (store, i->x509 ())) {
			X509_STORE_free (store);
			return false;
		}

		X509_STORE_CTX* ctx = X509_STORE_CTX_new ();
		if (!ctx) {
			X509_STORE_free (store);
			return false;
		}

		X509_STORE_set_flags (store, 0);
		if (!X509_STORE_CTX_init (ctx, store, j->x509 (), 0)) {
			X509_STORE_CTX_free (ctx);
			X509_STORE_free (store);
			return false;
		}

		int v = X509_verify_cert (ctx);
		X509_STORE_CTX_free (ctx);

		if (v == 0) {
			X509_STORE_free (store);
			return false;
		}
	}

	X509_STORE_free (store);

	/* Check that the leaf certificate matches the private key, if there is one */

	if (!_key) {
		return true;
	}

	BIO* bio = BIO_new_mem_buf (const_cast<char *> (_key->c_str ()), -1);
	if (!bio) {
		throw MiscError ("could not create memory BIO");
	}

	RSA* private_key = PEM_read_bio_RSAPrivateKey (bio, 0, 0, 0);
	RSA* public_key = leaf().public_key ();
	bool const valid = !BN_cmp (private_key->n, public_key->n);
	BIO_free (bio);

	return valid;
}

/** @return true if the chain is now in order from root to leaf,
 *  false if no correct order was found.
 */
bool
CertificateChain::attempt_reorder ()
{
	List original = _certificates;
	_certificates.sort ();
	do {
		if (valid ()) {
			return true;
		}
	} while (std::next_permutation (_certificates.begin(), _certificates.end ()));

	_certificates = original;
	return false;
}

/** Add a &lt;Signer&gt; and &lt;ds:Signature&gt; nodes to an XML node.
 *  @param parent XML node to add to.
 *  @param standard INTEROP or SMPTE.
 */
void
CertificateChain::sign (xmlpp::Element* parent, Standard standard) const
{
	/* <Signer> */

	xmlpp::Element* signer = parent->add_child("Signer");
	xmlpp::Element* data = signer->add_child("X509Data", "dsig");
	xmlpp::Element* serial_element = data->add_child("X509IssuerSerial", "dsig");
	serial_element->add_child("X509IssuerName", "dsig")->add_child_text (leaf().issuer());
	serial_element->add_child("X509SerialNumber", "dsig")->add_child_text (leaf().serial());
	data->add_child("X509SubjectName", "dsig")->add_child_text (leaf().subject());

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
CertificateChain::add_signature_value (xmlpp::Node* parent, string ns) const
{
	cxml::Node cp (parent);
	xmlpp::Node* key_info = cp.node_child("KeyInfo")->node ();

	/* Add the certificate chain to the KeyInfo child node of parent */
	BOOST_FOREACH (Certificate const & i, leaf_to_root ()) {
		xmlpp::Element* data = key_info->add_child("X509Data", ns);

		{
			xmlpp::Element* serial = data->add_child("X509IssuerSerial", ns);
			serial->add_child("X509IssuerName", ns)->add_child_text (i.issuer ());
			serial->add_child("X509SerialNumber", ns)->add_child_text (i.serial ());
		}

		data->add_child("X509Certificate", ns)->add_child_text (i.certificate());
	}

	xmlSecDSigCtxPtr signature_context = xmlSecDSigCtxCreate (0);
	if (signature_context == 0) {
		throw MiscError ("could not create signature context");
	}

	signature_context->signKey = xmlSecCryptoAppKeyLoadMemory (
		reinterpret_cast<const unsigned char *> (_key->c_str()), _key->size(), xmlSecKeyDataFormatPem, 0, 0, 0
		);

	if (signature_context->signKey == 0) {
		throw runtime_error ("could not read private key");
	}

	/* XXX: set key name to the PEM string: this can't be right! */
	if (xmlSecKeySetName (signature_context->signKey, reinterpret_cast<const xmlChar *> (_key->c_str())) < 0) {
		throw MiscError ("could not set key name");
	}

	int const r = xmlSecDSigCtxSign (signature_context, parent->cobj ());
	if (r < 0) {
		throw MiscError (String::compose ("could not sign (%1)", r));
	}

	xmlSecDSigCtxDestroy (signature_context);
}
