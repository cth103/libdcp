/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

/** @file  src/certificates.cc
 *  @brief Certificate and CertificateChain classes.
 */

#include "KM_util.h"
#include "certificates.h"
#include "compose.hpp"
#include "exceptions.h"
#include "util.h"
#include <libxml++/nodes/element.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/asn1.h>
#include <openssl/err.h>
#include <boost/algorithm/string.hpp>
#include <cerrno>
#include <algorithm>

using std::list;
using std::string;
using std::cout;
using boost::shared_ptr;
using namespace dcp;

/** @param c X509 certificate, which this object will take ownership of */
Certificate::Certificate (X509* c)
	: _certificate (c)
	, _public_key (0)
{
	
}

/** Load an X509 certificate from a string.
 *  @param cert String to read from.
 */
Certificate::Certificate (string cert)
	: _certificate (0)
	, _public_key (0)
{
	read_string (cert);
}

/** Copy constructor.
 *  @param other Certificate to copy.
 */
Certificate::Certificate (Certificate const & other)
	: _certificate (0)
	, _public_key (0)
{
	read_string (other.certificate (true));
}

/** Read a certificate from a string.
 *  @param cert String to read.
 */
void
Certificate::read_string (string cert)
{
	BIO* bio = BIO_new_mem_buf (const_cast<char *> (cert.c_str ()), -1);
	if (!bio) {
		throw MiscError ("could not create memory BIO");
	}

	_certificate = PEM_read_bio_X509 (bio, 0, 0, 0);
	if (!_certificate) {
		throw MiscError ("could not read X509 certificate from memory BIO");
	}

	BIO_free (bio);
}

/** Destructor */
Certificate::~Certificate ()
{
	X509_free (_certificate);
	RSA_free (_public_key);
}

/** operator= for Certificate.
 *  @param other Certificate to read from.
 */
Certificate &
Certificate::operator= (Certificate const & other)
{
	if (this == &other) {
		return *this;
	}

	X509_free (_certificate);
	_certificate = 0;
	RSA_free (_public_key);
	_public_key = 0;
	
	read_string (other.certificate (true));

	return *this;
}

/** Return the certificate as a string.
 *  @param with_begin_end true to include the -----BEGIN CERTIFICATE--- / -----END CERTIFICATE----- markers.
 *  @return Certificate string.
 */
string
Certificate::certificate (bool with_begin_end) const
{
	assert (_certificate);
	
	BIO* bio = BIO_new (BIO_s_mem ());
	if (!bio) {
		throw MiscError ("could not create memory BIO");
	}
	
	PEM_write_bio_X509 (bio, _certificate);

	string s;
	char* data;
	long int const data_length = BIO_get_mem_data (bio, &data);
	for (long int i = 0; i < data_length; ++i) {
		s += data[i];
	}

	BIO_free (bio);

	if (!with_begin_end) {
		boost::replace_all (s, "-----BEGIN CERTIFICATE-----\n", "");
		boost::replace_all (s, "\n-----END CERTIFICATE-----\n", "");
	}
	
	return s;
}

/** @return Certificate's issuer, in the form
 *  dnqualifier=&lt;dnQualififer&gt;,CN=&lt;commonName&gt;,OU=&lt;organizationalUnitName&gt,O=&lt;organizationName&gt;
 *  and with + signs escaped to \+
 */
string
Certificate::issuer () const
{
	assert (_certificate);
	return name_for_xml (X509_get_issuer_name (_certificate));
}

string
Certificate::asn_to_utf8 (ASN1_STRING* s)
{
	unsigned char* buf = 0;
	ASN1_STRING_to_UTF8 (&buf, s);
	string const u (reinterpret_cast<char *> (buf));
	OPENSSL_free (buf);
	return u;
}

string
Certificate::get_name_part (X509_NAME* n, int nid)
{
	int p = -1;
	p = X509_NAME_get_index_by_NID (n, nid, p);
	assert (p != -1);
	return asn_to_utf8 (X509_NAME_ENTRY_get_data (X509_NAME_get_entry (n, p)));
}
	

string
Certificate::name_for_xml (X509_NAME * n)
{
	assert (n);

	string s = String::compose (
		"dnQualifier=%1,CN=%2,OU=%3,O=%4",
		get_name_part (n, NID_dnQualifier),
		get_name_part (n, NID_commonName),
		get_name_part (n, NID_organizationalUnitName),
		get_name_part (n, NID_organizationName)
		);
	
	boost::replace_all (s, "+", "\\+");
	return s;
}

string
Certificate::subject () const
{
	assert (_certificate);

	return name_for_xml (X509_get_subject_name (_certificate));
}

string
Certificate::common_name () const
{
	assert (_certificate);

	return get_name_part (X509_get_subject_name (_certificate), NID_commonName);
}

string
Certificate::serial () const
{
	assert (_certificate);

	ASN1_INTEGER* s = X509_get_serialNumber (_certificate);
	assert (s);
	
	BIGNUM* b = ASN1_INTEGER_to_BN (s, 0);
	char* c = BN_bn2dec (b);
	BN_free (b);
	
	string st (c);
	OPENSSL_free (c);

	return st;
}

string
Certificate::thumbprint () const
{
	assert (_certificate);
	
	uint8_t buffer[8192];
	uint8_t* p = buffer;
	i2d_X509_CINF (_certificate->cert_info, &p);
	unsigned int const length = p - buffer;
	if (length > sizeof (buffer)) {
		throw MiscError ("buffer too small to generate thumbprint");
	}

	SHA_CTX sha;
	SHA1_Init (&sha);
	SHA1_Update (&sha, buffer, length);
	uint8_t digest[20];
	SHA1_Final (digest, &sha);

	char digest_base64[64];
	return Kumu::base64encode (digest, 20, digest_base64, 64);
}

/** @return RSA public key from this Certificate.  Caller must not free the returned value. */
RSA *
Certificate::public_key () const
{
	assert (_certificate);

	if (_public_key) {
		return _public_key;
	}

	EVP_PKEY* key = X509_get_pubkey (_certificate);
	if (!key) {
		throw MiscError ("could not get public key from certificate");
	}

	_public_key = EVP_PKEY_get1_RSA (key);
	if (!_public_key) {
		throw MiscError (String::compose ("could not get RSA public key (%1)", ERR_error_string (ERR_get_error(), 0)));
	}

	return _public_key;
}

/** @return Root certificate */
shared_ptr<const Certificate>
CertificateChain::root () const
{
	assert (!_certificates.empty());
	return _certificates.front ();
}

/** @return Leaf certificate */
shared_ptr<const Certificate>
CertificateChain::leaf () const
{
	assert (_certificates.size() >= 2);
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
CertificateChain::add (shared_ptr<const Certificate> c)
{
	_certificates.push_back (c);
}

/** Remove a certificate from the chain.
 *  @param c Certificate to remove.
 */
void
CertificateChain::remove (shared_ptr<const Certificate> c)
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
 *  signs the leaf and so on).
 *  @return true if it's ok, false if not.
 */
bool
CertificateChain::valid () const
{
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

		if (!X509_STORE_add_cert (store, (*i)->x509 ())) {
			X509_STORE_free (store);
			return false;
		}

		X509_STORE_CTX* ctx = X509_STORE_CTX_new ();
		if (!ctx) {
			X509_STORE_free (store);
			return false;
		}

		X509_STORE_set_flags (store, 0);
		if (!X509_STORE_CTX_init (ctx, store, (*j)->x509 (), 0)) {
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
	return true;
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
