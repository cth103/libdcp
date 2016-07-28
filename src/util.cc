/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

/** @file  src/util.cc
 *  @brief Utility methods.
 */

#include "util.h"
#include "exceptions.h"
#include "types.h"
#include "certificate.h"
#include "openjpeg_image.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include <asdcp/KM_util.h>
#include <asdcp/KM_fileio.h>
#include <asdcp/AS_DCP.h>
#include <xmlsec/xmldsig.h>
#include <xmlsec/dl.h>
#include <xmlsec/app.h>
#include <xmlsec/crypto.h>
#include <libxml++/nodes/element.h>
#include <libxml++/document.h>
#include <openssl/sha.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <stdexcept>
#include <iostream>
#include <iomanip>

using std::string;
using std::wstring;
using std::cout;
using std::min;
using std::max;
using std::list;
using std::setw;
using std::setfill;
using std::ostream;
using boost::shared_ptr;
using boost::shared_array;
using boost::optional;
using boost::function;
using boost::algorithm::trim;
using namespace dcp;

/** Create a UUID.
 *  @return UUID.
 */
string
dcp::make_uuid ()
{
	char buffer[64];
	Kumu::UUID id;
	Kumu::GenRandomValue (id);
	id.EncodeHex (buffer, 64);
	return string (buffer);
}


/** Create a digest for a file.
 *  @param filename File name.
 *  @param progress Optional progress reporting function.  The function will be called
 *  with a progress value between 0 and 1.
 *  @return Digest.
 */
string
dcp::make_digest (boost::filesystem::path filename, function<void (float)> progress)
{
	Kumu::FileReader reader;
	Kumu::Result_t r = reader.OpenRead (filename.string().c_str ());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (FileError ("could not open file to compute digest", filename, r));
	}

	SHA_CTX sha;
	SHA1_Init (&sha);

	int const buffer_size = 65536;
	Kumu::ByteString read_buffer (buffer_size);

	Kumu::fsize_t done = 0;
	Kumu::fsize_t const size = reader.Size ();
	while (1) {
		ui32_t read = 0;
		Kumu::Result_t r = reader.Read (read_buffer.Data(), read_buffer.Capacity(), &read);

		if (r == Kumu::RESULT_ENDOFFILE) {
			break;
		} else if (ASDCP_FAILURE (r)) {
			boost::throw_exception (FileError ("could not read file to compute digest", filename, r));
		}

		SHA1_Update (&sha, read_buffer.Data(), read);

		if (progress) {
			progress (float (done) / size);
			done += read;
		}
	}

	byte_t byte_buffer[SHA_DIGEST_LENGTH];
	SHA1_Final (byte_buffer, &sha);

	char digest[64];
	return Kumu::base64encode (byte_buffer, SHA_DIGEST_LENGTH, digest, 64);
}

/** Convert a content kind to a string which can be used in a
 *  &lt;ContentKind&gt; node.
 *  @param kind ContentKind.
 *  @return string.
 */
string
dcp::content_kind_to_string (ContentKind kind)
{
	switch (kind) {
	case FEATURE:
		return "feature";
	case SHORT:
		return "short";
	case TRAILER:
		return "trailer";
	case TEST:
		return "test";
	case TRANSITIONAL:
		return "transitional";
	case RATING:
		return "rating";
	case TEASER:
		return "teaser";
	case POLICY:
		return "policy";
	case PUBLIC_SERVICE_ANNOUNCEMENT:
		return "psa";
	case ADVERTISEMENT:
		return "advertisement";
	}

	DCP_ASSERT (false);
}

/** Convert a string from a &lt;ContentKind&gt; node to a libdcp ContentKind.
 *  Reasonably tolerant about varying case.
 *  @param kind Content kind string.
 *  @return libdcp ContentKind.
 */
dcp::ContentKind
dcp::content_kind_from_string (string kind)
{
	transform (kind.begin(), kind.end(), kind.begin(), ::tolower);

	if (kind == "feature") {
		return FEATURE;
	} else if (kind == "short") {
		return SHORT;
	} else if (kind == "trailer") {
		return TRAILER;
	} else if (kind == "test") {
		return TEST;
	} else if (kind == "transitional") {
		return TRANSITIONAL;
	} else if (kind == "rating") {
		return RATING;
	} else if (kind == "teaser") {
		return TEASER;
	} else if (kind == "policy") {
		return POLICY;
	} else if (kind == "psa") {
		return PUBLIC_SERVICE_ANNOUNCEMENT;
	} else if (kind == "advertisement") {
		return ADVERTISEMENT;
	}

	DCP_ASSERT (false);
}

/** @param s A string.
 *  @return true if the string contains only space, newline or tab characters, or is empty.
 */
bool
dcp::empty_or_white_space (string s)
{
	for (size_t i = 0; i < s.length(); ++i) {
		if (s[i] != ' ' && s[i] != '\n' && s[i] != '\t') {
			return false;
		}
	}

	return true;
}

/** Set up various bits that the library needs.  Should be called one
 *  by client applications.
 */
void
dcp::init ()
{
	if (xmlSecInit() < 0) {
		throw MiscError ("could not initialise xmlsec");
	}

#ifdef XMLSEC_CRYPTO_DYNAMIC_LOADING
	if (xmlSecCryptoDLLoadLibrary(BAD_CAST XMLSEC_CRYPTO) < 0) {
		throw MiscError ("unable to load default xmlsec-crypto library");
	}
#endif

	if (xmlSecCryptoAppInit(0) < 0) {
		throw MiscError ("could not initialise crypto");
	}

	if (xmlSecCryptoInit() < 0) {
		throw MiscError ("could not initialise xmlsec-crypto");
	}
}

bool dcp::operator== (dcp::Size const & a, dcp::Size const & b)
{
	return (a.width == b.width && a.height == b.height);
}

bool dcp::operator!= (dcp::Size const & a, dcp::Size const & b)
{
	return !(a == b);
}

ostream& dcp::operator<< (ostream& s, dcp::Size const & a)
{
	s << a.width << "x" << a.height;
	return s;
}

/** Decode a base64 string.  The base64 decode routine in KM_util.cpp
 *  gives different values to both this and the command-line base64
 *  for some inputs.  Not sure why.
 *
 *  @param in base64-encoded string.
 *  @param out Output buffer.
 *  @param out_length Length of output buffer.
 *  @return Number of characters written to the output buffer.
 */
int
dcp::base64_decode (string const & in, unsigned char* out, int out_length)
{
	BIO* b64 = BIO_new (BIO_f_base64 ());

	/* This means the input should have no newlines */
	BIO_set_flags (b64, BIO_FLAGS_BASE64_NO_NL);

	/* Copy our input string, removing newlines */
	char in_buffer[in.size() + 1];
	char* p = in_buffer;
	for (size_t i = 0; i < in.size(); ++i) {
		if (in[i] != '\n' && in[i] != '\r') {
			*p++ = in[i];
		}
	}

	BIO* bmem = BIO_new_mem_buf (in_buffer, p - in_buffer);
	bmem = BIO_push (b64, bmem);
	int const N = BIO_read (bmem, out, out_length);
	BIO_free_all (bmem);

	return N;
}

/** @param p Path to open.
 *  @param t mode flags, as for fopen(3).
 *  @return FILE pointer or 0 on error.
 *
 *  Apparently there is no way to create an ofstream using a UTF-8
 *  filename under Windows.  We are hence reduced to using fopen
 *  with this wrapper.
 */
FILE *
dcp::fopen_boost (boost::filesystem::path p, string t)
{
#ifdef LIBDCP_WINDOWS
        wstring w (t.begin(), t.end());
	/* c_str() here should give a UTF-16 string */
        return _wfopen (p.c_str(), w.c_str ());
#else
        return fopen (p.c_str(), t.c_str ());
#endif
}

optional<boost::filesystem::path>
dcp::relative_to_root (boost::filesystem::path root, boost::filesystem::path file)
{
	boost::filesystem::path::const_iterator i = root.begin ();
	boost::filesystem::path::const_iterator j = file.begin ();

	while (i != root.end() && j != file.end() && *i == *j) {
		++i;
		++j;
	}

	if (i != root.end ()) {
		return optional<boost::filesystem::path> ();
	}

	boost::filesystem::path rel;
	while (j != file.end ()) {
		rel /= *j++;
	}

	return rel;
}

bool
dcp::ids_equal (string a, string b)
{
	transform (a.begin(), a.end(), a.begin(), ::tolower);
	transform (b.begin(), b.end(), b.begin(), ::tolower);
	trim (a);
	trim (b);
	return a == b;
}

string
dcp::file_to_string (boost::filesystem::path p, uintmax_t max_length)
{
	uintmax_t len = boost::filesystem::file_size (p);
	if (len > max_length) {
		throw MiscError ("Unexpectedly long file");
	}

	FILE* f = fopen_boost (p, "r");
	if (!f) {
		throw FileError ("could not open file", p, errno);
	}

	char* c = new char[len];
	/* This may read less than `len' if we are on Windows and we have CRLF in the file */
	int const N = fread (c, 1, len, f);
	fclose (f);

	string s (c, N);
	delete[] c;

	return s;
}

/** @param key RSA private key in PEM format (optionally with -----BEGIN... / -----END...)
 *  @return SHA1 fingerprint of key
 */
string
dcp::private_key_fingerprint (string key)
{
	boost::replace_all (key, "-----BEGIN RSA PRIVATE KEY-----\n", "");
	boost::replace_all (key, "\n-----END RSA PRIVATE KEY-----\n", "");

	unsigned char buffer[4096];
	int const N = base64_decode (key, buffer, sizeof (buffer));

	SHA_CTX sha;
	SHA1_Init (&sha);
	SHA1_Update (&sha, buffer, N);
	uint8_t digest[20];
	SHA1_Final (digest, &sha);

	char digest_base64[64];
	return Kumu::base64encode (digest, 20, digest_base64, 64);
}

xmlpp::Node *
dcp::find_child (xmlpp::Node const * node, string name)
{
	xmlpp::Node::NodeList c = node->get_children ();
	xmlpp::Node::NodeList::iterator i = c.begin();
	while (i != c.end() && (*i)->get_name() != name) {
		++i;
	}

	DCP_ASSERT (i != c.end ());
	return *i;
}

string
dcp::remove_urn_uuid (string raw)
{
	DCP_ASSERT (raw.substr(0, 9) == "urn:uuid:");
	return raw.substr (9);
}
