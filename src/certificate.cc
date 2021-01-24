/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/certificate.cc
 *  @brief Certificate class
 */


#include "certificate.h"
#include "compose.hpp"
#include "exceptions.h"
#include "util.h"
#include "dcp_assert.h"
#include <asdcp/KM_util.h>
#include <libxml++/nodes/element.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/asn1.h>
#include <openssl/err.h>
#include <boost/algorithm/string.hpp>
#include <cerrno>
#include <iostream>
#include <algorithm>


using std::list;
using std::string;
using std::ostream;
using std::min;
using namespace dcp;


static string const begin_certificate = "-----BEGIN CERTIFICATE-----";
static string const end_certificate = "-----END CERTIFICATE-----";


Certificate::Certificate (X509* c)
	: _certificate (c)
{

}


Certificate::Certificate (string cert)
{
	auto const s = read_string (cert);
	if (!s.empty()) {
		throw MiscError ("unexpected data after certificate");
	}
}


Certificate::Certificate (Certificate const & other)
{
	if (other._certificate) {
		read_string (other.certificate (true));
	}
}


string
Certificate::read_string (string cert)
{
	/* Reformat cert so that it has line breaks every 64 characters.
	   See http://comments.gmane.org/gmane.comp.encryption.openssl.user/55593
	*/

	list<string> lines;
	string line;

	for (size_t i = 0; i < cert.length(); ++i) {
		line += cert[i];
		if (cert[i] == '\r' || cert[i] == '\n') {
			boost::algorithm::trim (line);
			lines.push_back (line);
			line = "";
		}
	}

	if (!line.empty()) {
		boost::algorithm::trim (line);
		lines.push_back (line);
	}

	auto i = lines.begin ();

	/* BEGIN */
	while (i != lines.end() && *i != begin_certificate) {
		++i;
	}

	if (i == lines.end()) {
		throw MiscError ("missing BEGIN line in certificate");
	}

	/* Skip over the BEGIN line */
	++i;

	/* The base64 data */
	bool got_end = false;
	string base64 = "";
	while (i != lines.end()) {
		if (*i == end_certificate) {
			got_end = true;
			break;
		}
		base64 += *i;
		++i;
	}

	if (!got_end) {
		throw MiscError ("missing END line in certificate");
	}

	/* Skip over the END line */
	++i;

	/* Make up the fixed version */

	string fixed = begin_certificate + "\n";
	while (!base64.empty ()) {
		size_t const t = min (size_t(64), base64.length());
		fixed += base64.substr (0, t) + "\n";
		base64 = base64.substr (t, base64.length() - t);
	}

	fixed += end_certificate;

	auto bio = BIO_new_mem_buf (const_cast<char *> (fixed.c_str ()), -1);
	if (!bio) {
		throw MiscError ("could not create memory BIO");
	}

	_certificate = PEM_read_bio_X509 (bio, 0, 0, 0);
	if (!_certificate) {
		throw MiscError ("could not read X509 certificate from memory BIO");
	}

	BIO_free (bio);

	string extra;

	while (i != lines.end()) {
		if (!i->empty()) {
			extra += *i + "\n";
		}
		++i;
	}

	return extra;
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

	read_string (other.certificate(true));

	return *this;
}


string
Certificate::certificate (bool with_begin_end) const
{
	DCP_ASSERT (_certificate);

	auto bio = BIO_new (BIO_s_mem());
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
		boost::replace_all (s, begin_certificate + "\n", "");
		boost::replace_all (s, "\n" + end_certificate + "\n", "");
	}

	return s;
}


string
Certificate::issuer () const
{
	DCP_ASSERT (_certificate);
	return name_for_xml (X509_get_issuer_name(_certificate));
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
	if (p == -1) {
		return "";
	}
	return asn_to_utf8 (X509_NAME_ENTRY_get_data(X509_NAME_get_entry(n, p)));
}


string
Certificate::name_for_xml (X509_NAME* name)
{
	assert (name);

	auto bio = BIO_new (BIO_s_mem ());
	if (!bio) {
		throw MiscError ("could not create memory BIO");
	}

	X509_NAME_print_ex (bio, name, 0, XN_FLAG_RFC2253);
	int n = BIO_pending (bio);
	char* result = new char[n + 1];
	n = BIO_read (bio, result, n);
	result[n] = '\0';

	BIO_free (bio);

	string s = result;
	delete[] result;

	return s;
}

string
Certificate::subject () const
{
	DCP_ASSERT (_certificate);

	return name_for_xml (X509_get_subject_name(_certificate));
}


string
Certificate::subject_common_name () const
{
	DCP_ASSERT (_certificate);

	return get_name_part (X509_get_subject_name(_certificate), NID_commonName);
}


string
Certificate::subject_organization_name () const
{
	DCP_ASSERT (_certificate);

	return get_name_part (X509_get_subject_name(_certificate), NID_organizationName);
}


string
Certificate::subject_organizational_unit_name () const
{
	DCP_ASSERT (_certificate);

	return get_name_part (X509_get_subject_name(_certificate), NID_organizationalUnitName);
}


static
struct tm
convert_time (ASN1_TIME const * time)
{
	struct tm t;
	char const * s = (char const *) time->data;

	if (time->type == V_ASN1_UTCTIME) {
		sscanf(s, "%2d%2d%2d%2d%2d%2d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec);
		if (t.tm_year < 70) {
			t.tm_year += 100;
		}
	} else if (time->type == V_ASN1_GENERALIZEDTIME) {
		sscanf(s, "%4d%2d%2d%2d%2d%2d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec);
		t.tm_year -= 1900;
	}

	t.tm_mon--;

	return t;
}


struct tm
Certificate::not_before () const
{
	DCP_ASSERT (_certificate);
#if OPENSSL_VERSION_NUMBER > 0x10100000L
	return convert_time(X509_get0_notBefore(_certificate));
#else
	return convert_time(X509_get_notBefore(_certificate));
#endif
}


struct tm
Certificate::not_after () const
{
	DCP_ASSERT (_certificate);
#if OPENSSL_VERSION_NUMBER > 0x10100000L
	return convert_time(X509_get0_notAfter(_certificate));
#else
	return convert_time(X509_get_notAfter(_certificate));
#endif
}


string
Certificate::serial () const
{
	DCP_ASSERT (_certificate);

	auto s = X509_get_serialNumber (_certificate);
	DCP_ASSERT (s);

	auto b = ASN1_INTEGER_to_BN (s, 0);
	char* c = BN_bn2dec (b);
	BN_free (b);

	string st (c);
	OPENSSL_free (c);

	return st;
}


string
Certificate::thumbprint () const
{
	DCP_ASSERT (_certificate);

	uint8_t buffer[8192];
	uint8_t* p = buffer;

#if OPENSSL_VERSION_NUMBER > 0x10100000L
	i2d_re_X509_tbs(_certificate, &p);
#else
	i2d_X509_CINF (_certificate->cert_info, &p);
#endif
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


RSA *
Certificate::public_key () const
{
	DCP_ASSERT (_certificate);

	if (_public_key) {
		return _public_key;
	}

	auto key = X509_get_pubkey (_certificate);
	if (!key) {
		throw MiscError ("could not get public key from certificate");
	}

	_public_key = EVP_PKEY_get1_RSA (key);
	if (!_public_key) {
		throw MiscError (String::compose ("could not get RSA public key (%1)", ERR_error_string (ERR_get_error(), 0)));
	}

	return _public_key;
}


static bool string_is_utf8 (X509_NAME* n, int nid)
{
	int p = -1;
	p = X509_NAME_get_index_by_NID (n, nid, p);
	return p != -1 && X509_NAME_ENTRY_get_data(X509_NAME_get_entry(n, p))->type == V_ASN1_UTF8STRING;
}


bool
Certificate::has_utf8_strings () const
{
	auto n = X509_get_subject_name (_certificate);
	return string_is_utf8(n, NID_commonName) ||
		string_is_utf8(n, NID_organizationName) ||
		string_is_utf8(n, NID_organizationalUnitName);
}


bool
dcp::operator== (Certificate const & a, Certificate const & b)
{
	return a.certificate() == b.certificate();
}


bool
dcp::operator< (Certificate const & a, Certificate const & b)
{
	return a.certificate() < b.certificate();
}


ostream&
dcp::operator<< (ostream& s, Certificate const & c)
{
	s << c.certificate();
	return s;
}
