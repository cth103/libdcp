/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "KM_util.h"
#include "signer_chain.h"
#include "exceptions.h"
#include "util.h"

using std::string;
using std::ofstream;
using std::ifstream;
using std::stringstream;
using std::cout;

static void command (string cmd)
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
		stringstream s;
		s << "error " << code << " in " << cmd << " within " << boost::filesystem::current_path();
		throw libdcp::MiscError (s.str());
	}
}

/** Extract a public key from a private key and create a SHA1 digest of it.
 *  @param key Private key
 *  @param openssl openssl binary name (or full path if openssl is not on the system path).
 *  @return SHA1 digest of corresponding public key, with escaped / characters.
 */
	
static string
public_key_digest (boost::filesystem::path private_key, boost::filesystem::path openssl)
{
	boost::filesystem::path public_name = private_key.string() + ".public";

	/* Create the public key from the private key */
	stringstream s;
	s << "\"" << openssl.string() << "\" rsa -outform PEM -pubout -in " << private_key.string() << " -out " << public_name.string ();
	command (s.str().c_str ());

	/* Read in the public key from the file */

	string pub;
	ifstream f (public_name.string().c_str ());
	if (!f.good ()) {
		throw libdcp::MiscError ("public key not found");
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
	int const N = libdcp::base64_decode (pub, buffer, 1024);

	/* Hash it with SHA1 (without the first 24 bytes, for reasons that are not entirely clear) */

	SHA_CTX context;
	if (!SHA1_Init (&context)) {
		throw libdcp::MiscError ("could not init SHA1 context");
	}

	if (!SHA1_Update (&context, buffer + 24, N - 24)) {
		throw libdcp::MiscError ("could not update SHA1 digest");
	}

	unsigned char digest[SHA_DIGEST_LENGTH];
	if (!SHA1_Final (digest, &context)) {
		throw libdcp::MiscError ("could not finish SHA1 digest");
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

void
libdcp::make_signer_chain (boost::filesystem::path directory, boost::filesystem::path openssl)
{
	boost::filesystem::path const cwd = boost::filesystem::current_path ();

	string quoted_openssl = "\"" + openssl.string() + "\"";

	boost::filesystem::current_path (directory);
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

	string const ca_subject = "/O=example.org/OU=example.org/CN=.smpte-430-2.ROOT.NOT_FOR_PRODUCTION/dnQualifier=" + public_key_digest ("ca.key", openssl);

	{
		stringstream c;
		c << quoted_openssl
		  << " req -new -x509 -sha256 -config ca.cnf -days 3650 -set_serial 5"
		  << " -subj " << ca_subject << " -key ca.key -outform PEM -out ca.self-signed.pem";
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
		
	string const inter_subject = "/O=example.org/OU=example.org/CN=.smpte-430-2.INTERMEDIATE.NOT_FOR_PRODUCTION/dnQualifier="
		+ public_key_digest ("intermediate.key", openssl);

	{
		stringstream s;
		s << quoted_openssl
		  << " req -new -config intermediate.cnf -days 3649 -subj " << inter_subject << " -key intermediate.key -out intermediate.csr";
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

	string const leaf_subject = "/O=example.org/OU=example.org/CN=CS.smpte-430-2.LEAF.NOT_FOR_PRODUCTION/dnQualifier="
		+ public_key_digest ("leaf.key", openssl);

	{
		stringstream s;
		s << quoted_openssl << " req -new -config leaf.cnf -days 3648 -subj " << leaf_subject << " -key leaf.key -outform PEM -out leaf.csr";
		command (s.str().c_str());
	}

	command (
		quoted_openssl +
		" x509 -req -sha256 -days 3648 -CA intermediate.signed.pem -CAkey intermediate.key"
		" -set_serial 7 -in leaf.csr -extfile leaf.cnf -extensions v3_ca -out leaf.signed.pem"
		);

	boost::filesystem::current_path (cwd);
}
