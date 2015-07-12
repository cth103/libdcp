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
 *  This class can take a Certificate from a string or an OpenSSL X509 object.
 */
class Certificate
{
public:
	Certificate ()
		: _certificate (0)
		, _public_key (0)
	{}

	Certificate (std::string);
	Certificate (X509 *);
	Certificate (Certificate const &);
	~Certificate ();

	Certificate& operator= (Certificate const &);

	std::string certificate (bool with_begin_end = false) const;
	std::string serial () const;

	std::string issuer () const;

	std::string subject () const;
	std::string subject_common_name () const;
	std::string subject_organization_name () const;
	std::string subject_organizational_unit_name () const;

	X509* x509 () const {
		return _certificate;
	}

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

bool operator== (Certificate const & a, Certificate const & b);
bool operator< (Certificate const & a, Certificate const & b);
std::ostream& operator<< (std::ostream&s, Certificate const & c);

/** @class CertificateChain
 *  @brief A chain of any number of certificates, from root to leaf.
 */
class CertificateChain
{
public:
	CertificateChain () {}

	void add (Certificate c);
	void remove (Certificate c);
	void remove (int);

	Certificate root () const;
	Certificate leaf () const;

	typedef std::list<Certificate> List;

	List leaf_to_root () const;
	List root_to_leaf () const;

	bool valid () const;
	bool attempt_reorder ();

private:
	friend class ::certificates;

	List _certificates;
};

}

#endif
