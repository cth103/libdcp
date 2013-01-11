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
#include <boost/algorithm/string.hpp>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/asn1.h>
#include <libxml++/nodes/element.h>
#include "KM_util.h"
#include "certificates.h"
#include "exceptions.h"

using std::list;
using std::string;
using std::stringstream;
using std::vector;
using boost::shared_ptr;
using namespace libdcp;

/** @param c X509 certificate, which this object will take ownership of */
Certificate::Certificate (X509* c)
	: _certificate (c)
{
	
}

Certificate::Certificate (string const & filename)
	: _certificate (0)
{
	FILE* f = fopen (filename.c_str(), "r");
	if (!f) {
		throw FileError ("could not open file", filename);
	}
	
	if (!PEM_read_X509 (f, &_certificate, 0, 0)) {
		throw MiscError ("could not read X509 certificate");
	}
}

Certificate::~Certificate ()
{
	X509_free (_certificate);
}

string
Certificate::certificate () const
{
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

	boost::replace_all (s, "-----BEGIN CERTIFICATE-----\n", "");
	boost::replace_all (s, "\n-----END CERTIFICATE-----\n", "");
	return s;
}

string
Certificate::issuer () const
{
	X509_NAME* n = X509_get_issuer_name (_certificate);
	assert (n);

	char b[256];
	X509_NAME_oneline (n, b, 256);
	return b;
}

string
Certificate::name_for_xml (string const & n)
{
	stringstream x;
	
	vector<string> p;
	boost::split (p, n, boost::is_any_of ("/"));
	for (vector<string>::const_reverse_iterator i = p.rbegin(); i != p.rend(); ++i) {
		x << *i << ",";
	}

	string s = x.str();
	boost::replace_all (s, "+", "\\+");

	return s.substr(0, s.length() - 2);
}

string
Certificate::subject () const
{
	X509_NAME* n = X509_get_subject_name (_certificate);
	assert (n);

	char b[256];
	X509_NAME_oneline (n, b, 256);
	return b;
}

string
Certificate::serial () const
{
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
