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
#include "crypt_chain.h"
#include "exceptions.h"

using std::string;
using std::ofstream;
using std::ifstream;
using std::stringstream;
using std::cout;

static void command (char const * c)
{
	int const r = system (c);
	if (r) {
		stringstream s;
		s << "error in " << c << "\n";
		throw libdcp::MiscError (s.str());
	}
}

void
libdcp::make_crypt_chain (boost::filesystem::path directory)
{
	boost::filesystem::current_path (directory);
	command ("openssl genrsa -out ca.key 2048");

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

	command ("openssl rsa -outform PEM -pubout -in ca.key | openssl base64 -d | dd bs=1 skip=24 2>/dev/null | openssl sha1 -binary | openssl base64 > ca_dnq");

	string ca_dnq;

	{
		ifstream f ("ca_dnq");
		getline (f, ca_dnq);
		/* XXX: is this right? */
		boost::replace_all (ca_dnq, "/", "\\\\/");
	}
	
	string const ca_subject = "/O=example.org/OU=example.org/CN=.smpte-430-2.ROOT.NOT_FOR_PRODUCTION/dnQualifier=" + ca_dnq;

	{
		stringstream c;
		c << "openssl req -new -x509 -sha256 -config ca.cnf -days 3650 -set_serial 5 -subj " << ca_subject << " -key ca.key -outform PEM -out ca.self-signed.pem";
		command (c.str().c_str());
	}

	command ("openssl genrsa -out intermediate.key 2048");

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

	command ("openssl rsa -outform PEM -pubout -in intermediate.key | openssl base64 -d | dd bs=1 skip=24 2>/dev/null | openssl sha1 -binary | openssl base64 > inter_dnq");
	
	string inter_dnq;

	{
		ifstream f ("inter_dnq");
		getline (f, inter_dnq);
		boost::replace_all (inter_dnq, "/", "\\\\/");
	}
		
	string const inter_subject = "/O=example.org/OU=example.org/CN=.smpte-430-2.INTERMEDIATE.NOT_FOR_PRODUCTION/dnQualifier=" + inter_dnq;

	{
		stringstream s;
		s << "openssl req -new -config intermediate.cnf -days 3649 -subj " << inter_subject << " -key intermediate.key -out intermediate.csr";
		command (s.str().c_str());
	}

	
	command ("openssl x509 -req -sha256 -days 3649 -CA ca.self-signed.pem -CAkey ca.key -set_serial 6 -in intermediate.csr -extfile intermediate.cnf -extensions v3_ca -out intermediate.signed.pem");

	command ("openssl genrsa -out leaf.key 2048");

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

	command ("openssl rsa -outform PEM -pubout -in leaf.key | openssl base64 -d | dd bs=1 skip=24 2>/dev/null | openssl sha1 -binary | openssl base64 > leaf_dnq");
	
	string leaf_dnq;

	{
		ifstream f ("leaf_dnq");
		getline (f, leaf_dnq);
		boost::replace_all (leaf_dnq, "/", "\\\\/");
	}

	string const leaf_subject = "/O=example.org/OU=example.org/CN=CS.smpte-430-2.LEAF.NOT_FOR_PRODUCTION/dnQualifier=" + leaf_dnq;

	{
		stringstream s;
		s << "openssl req -new -config leaf.cnf -days 3648 -subj " << leaf_subject << " -key leaf.key -outform PEM -out leaf.csr";
		command (s.str().c_str());
	}

	command ("openssl x509 -req -sha256 -days 3648 -CA intermediate.signed.pem -CAkey intermediate.key -set_serial 7 -in leaf.csr -extfile leaf.cnf -extensions v3_ca -out leaf.signed.pem");
}
