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


/** @file  src/util.cc
 *  @brief Utility methods and classes
 */


#include "util.h"
#include "language_tag.h"
#include "exceptions.h"
#include "types.h"
#include "certificate.h"
#include "openjpeg_image.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include <openjpeg.h>
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
#include <boost/algorithm/string.hpp>
#if BOOST_VERSION >= 106100
#include <boost/dll/runtime_symbol_info.hpp>
#endif
#include <boost/filesystem.hpp>
#include <stdexcept>
#include <iostream>
#include <iomanip>


using std::string;
using std::wstring;
using std::cout;
using std::min;
using std::max;
using std::setw;
using std::setfill;
using std::ostream;
using std::shared_ptr;
using std::vector;
using boost::shared_array;
using boost::optional;
using boost::function;
using boost::algorithm::trim;
using namespace dcp;


/* Some ASDCP objects store this as a *&, for reasons which are not
 * at all clear, so we have to keep this around forever.
 */
ASDCP::Dictionary const* dcp::asdcp_smpte_dict = nullptr;


string
dcp::make_uuid ()
{
	char buffer[64];
	Kumu::UUID id;
	Kumu::GenRandomValue (id);
	id.EncodeHex (buffer, 64);
	return string (buffer);
}


string
dcp::make_digest (ArrayData data)
{
	SHA_CTX sha;
	SHA1_Init (&sha);
	SHA1_Update (&sha, data.data(), data.size());
	byte_t byte_buffer[SHA_DIGEST_LENGTH];
	SHA1_Final (byte_buffer, &sha);
	char digest[64];
	return Kumu::base64encode (byte_buffer, SHA_DIGEST_LENGTH, digest, 64);
}


string
dcp::make_digest (boost::filesystem::path filename, function<void (float)> progress)
{
	Kumu::FileReader reader;
	auto r = reader.OpenRead (filename.string().c_str ());
	if (ASDCP_FAILURE(r)) {
		boost::throw_exception (FileError("could not open file to compute digest", filename, r));
	}

	SHA_CTX sha;
	SHA1_Init (&sha);

	int const buffer_size = 65536;
	Kumu::ByteString read_buffer (buffer_size);

	Kumu::fsize_t done = 0;
	Kumu::fsize_t const size = reader.Size ();
	while (true) {
		ui32_t read = 0;
		auto r = reader.Read (read_buffer.Data(), read_buffer.Capacity(), &read);

		if (r == Kumu::RESULT_ENDOFFILE) {
			break;
		} else if (ASDCP_FAILURE (r)) {
			boost::throw_exception (FileError("could not read file to compute digest", filename, r));
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


void
dcp::init (optional<boost::filesystem::path> tags_directory)
{
	if (xmlSecInit() < 0) {
		throw MiscError ("could not initialise xmlsec");
	}

#ifdef XMLSEC_CRYPTO_DYNAMIC_LOADING
	if (xmlSecCryptoDLLoadLibrary(BAD_CAST "openssl") < 0) {
		throw MiscError ("unable to load openssl xmlsec-crypto library");
	}
#endif

	if (xmlSecCryptoAppInit(0) < 0) {
		throw MiscError ("could not initialise crypto");
	}

	if (xmlSecCryptoInit() < 0) {
		throw MiscError ("could not initialise xmlsec-crypto");
	}

	OpenSSL_add_all_algorithms();

	asdcp_smpte_dict = &ASDCP::DefaultSMPTEDict();

	if (!tags_directory) {
		tags_directory = resources_directory() / "tags";
	}

	load_language_tag_lists (*tags_directory);
}


int
dcp::base64_decode (string const & in, unsigned char* out, int out_length)
{
	auto b64 = BIO_new (BIO_f_base64());

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

	auto bmem = BIO_new_mem_buf (in_buffer, p - in_buffer);
	bmem = BIO_push (b64, bmem);
	int const N = BIO_read (bmem, out, out_length);
	BIO_free_all (bmem);

	return N;
}


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
	auto i = root.begin ();
	auto j = file.begin ();

	while (i != root.end() && j != file.end() && *i == *j) {
		++i;
		++j;
	}

	if (i != root.end()) {
		return {};
	}

	boost::filesystem::path rel;
	while (j != file.end()) {
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
	auto len = boost::filesystem::file_size (p);
	if (len > max_length) {
		throw MiscError (String::compose("Unexpectedly long file (%1)", p.string()));
	}

	auto f = fopen_boost (p, "r");
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
	auto c = node->get_children ();
	auto i = c.begin();
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


string
dcp::openjpeg_version ()
{
	return opj_version ();
}


string
dcp::spaces (int n)
{
	string s = "";
	for (int i = 0; i < n; ++i) {
		s += " ";
	}
	return s;
}


void
dcp::indent (xmlpp::Element* element, int initial)
{
	xmlpp::Node* last = nullptr;
	for (auto n: element->get_children()) {
		auto e = dynamic_cast<xmlpp::Element*>(n);
		if (e) {
			element->add_child_text_before (e, "\n" + spaces(initial + 2));
			indent (e, initial + 2);
			last = n;
		}
	}
	if (last) {
		element->add_child_text (last, "\n" + spaces(initial));
	}
}


bool
dcp::day_less_than_or_equal (LocalTime a, LocalTime b)
{
	if (a.year() != b.year()) {
		return a.year() < b.year();
	}

	if (a.month() != b.month()) {
		return a.month() < b.month();
	}

	return a.day() <= b.day();
}


bool
dcp::day_greater_than_or_equal (LocalTime a, LocalTime b)
{
	if (a.year() != b.year()) {
		return a.year() > b.year();
	}

	if (a.month() != b.month()) {
		return a.month() > b.month();
	}

	return a.day() >= b.day();
}


string
dcp::unique_string (vector<string> existing, string base)
{
	int const max_tries = existing.size() + 1;
	for (int i = 0; i < max_tries; ++i) {
		string trial = String::compose("%1%2", base, i);
		if (find(existing.begin(), existing.end(), trial) == existing.end()) {
			return trial;
		}
	}

	DCP_ASSERT (false);
}


ASDCPErrorSuspender::ASDCPErrorSuspender ()
	: _old (Kumu::DefaultLogSink())
{
	_sink = new Kumu::EntryListLogSink(_log);
	Kumu::SetDefaultLogSink (_sink);
}


ASDCPErrorSuspender::~ASDCPErrorSuspender ()
{
	Kumu::SetDefaultLogSink (&_old);
	delete _sink;
}


boost::filesystem::path dcp::directory_containing_executable ()
{
#if BOOST_VERSION >= 106100
	return boost::filesystem::canonical(boost::dll::program_location().parent_path());
#else
	char buffer[PATH_MAX];
	ssize_t N = readlink ("/proc/self/exe", buffer, PATH_MAX);
	return boost::filesystem::path(string(buffer, N)).parent_path();
#endif
}


boost::filesystem::path dcp::resources_directory ()
{
	/* We need a way to specify the tags directory for running un-installed binaries */
	char* prefix = getenv("LIBDCP_RESOURCES");
	if (prefix) {
		return prefix;
	}

#if defined(LIBDCP_OSX)
	return directory_containing_executable().parent_path() / "Resources";
#elif defined(LIBDCP_WINDOWS)
	return directory_containing_executable().parent_path();
#else
	return directory_containing_executable().parent_path() / "share" / "libdcp";
#endif
}


