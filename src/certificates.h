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

/** @file  src/certificates.h
 *  @brief Certificate and CertificateChain classes.
 */

#ifndef LIBDCP_CERTIFICATES_H
#define LIBDCP_CERTIFICATES_H

#undef X509_NAME
#include <openssl/x509.h>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <list>

class certificates;

namespace xmlpp {
	class Element;
}

namespace dcp {

/** @class Certificate
 *  @brief A wrapper for an X509 certificate.
 *
 *  This class can take a Certificate from a file, a string or an OpenSSL X509 object.
 */
class Certificate
{
public:
	Certificate ()
		: _certificate (0)
	{}

	Certificate (boost::filesystem::path);
	Certificate (std::string);
	Certificate (X509 *);
	Certificate (Certificate const &);
	~Certificate ();

	Certificate& operator= (Certificate const &);

	std::string certificate (bool with_begin_end = false) const;
	std::string issuer () const;
	std::string serial () const;
	std::string subject () const;
	std::string common_name () const;

	RSA* public_key () const;

	std::string thumbprint () const;

private:
	void read_string (std::string);
	
	static std::string name_for_xml (X509_NAME *);
	static std::string asn_to_utf8 (ASN1_STRING *);
	static std::string get_name_part (X509_NAME *, int);

	X509* _certificate;
	mutable RSA* _public_key;
};

/** @class CertificateChain
 *  @brief A chain of any number of certificates, from root to leaf.
 */
class CertificateChain
{
public:
	CertificateChain () {}

	void add (boost::shared_ptr<Certificate> c);

	boost::shared_ptr<Certificate> root () const;
	boost::shared_ptr<Certificate> leaf () const;

	std::list<boost::shared_ptr<Certificate> > leaf_to_root () const;

private:
	friend class ::certificates;
	std::list<boost::shared_ptr<Certificate> > _certificates;
};

}

#endif
