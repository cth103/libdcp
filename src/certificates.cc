/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#include <sstream>
#include <vector>
#include <cerrno>
#include <boost/algorithm/string.hpp>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/asn1.h>
#include <openssl/err.h>
#include <libxml++/nodes/element.h>
#include "KM_util.h"
#include "certificates.h"
#include "compose.hpp"
#include "exceptions.h"
#include "util.h"

using std::list;
using std::string;
using std::stringstream;
using std::vector;
using boost::shared_ptr;
using namespace libdcp;

/** @param c X509 certificate, which this object will take ownership of */
Certificate::Certificate (X509* c)
	: _certificate (c)
	, _public_key (0)
{
	
}

Certificate::Certificate (boost::filesystem::path filename)
	: _certificate (0)
	, _public_key (0)
{
	FILE* f = fopen_boost (filename, "r");
	if (!f) {
		throw FileError ("could not open file", filename, errno);
	}
	
	if (!PEM_read_X509 (f, &_certificate, 0, 0)) {
		throw MiscError ("could not read X509 certificate");
	}
}

Certificate::Certificate (string cert)
	: _certificate (0)
	, _public_key (0)
{
	read_string (cert);
}

Certificate::Certificate (Certificate const & other)
	: _certificate (0)
	, _public_key (0)
{
	read_string (other.certificate (true));
}

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

Certificate::~Certificate ()
{
	X509_free (_certificate);
	RSA_free (_public_key);
}

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
	
	read_string (other.certificate ());

	return *this;
}

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
	int const length = p - buffer;
	if (length > 8192) {
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

shared_ptr<Certificate>
CertificateChain::root () const
{
	assert (!_certificates.empty());
	return _certificates.front ();
}

shared_ptr<Certificate>
CertificateChain::leaf () const
{
	assert (_certificates.size() >= 2);
	return _certificates.back ();
}

list<shared_ptr<Certificate> >
CertificateChain::leaf_to_root () const
{
	list<shared_ptr<Certificate> > c = _certificates;
	c.reverse ();
	return c;
}

void
CertificateChain::add (shared_ptr<Certificate> c)
{
	_certificates.push_back (c);
}
