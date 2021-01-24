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


/** @file  src/certificate.h
 *  @brief Certificate class
 */


#ifndef LIBDCP_CERTIFICATE_H
#define LIBDCP_CERTIFICATE_H


#undef X509_NAME
#include <openssl/x509.h>
#include <boost/filesystem.hpp>
#include <string>
#include <list>


namespace xmlpp {
	class Element;
}


namespace dcp {


/** @class Certificate
 *  @brief A wrapper for an X509 certificate
 *
 *  This class can take a Certificate from a string or an OpenSSL X509 object
 */
class Certificate
{
public:
	Certificate ()
		: _certificate (0)
		, _public_key (0)
	{}

	/** Load an X509 certificate from a string
	 *  @param cert String to read from
	 */
	explicit Certificate (std::string);

	/** @param c X509 certificate, which this object will take ownership of */
	explicit Certificate (X509 *);

	Certificate (Certificate const &);
	~Certificate ();

	Certificate& operator= (Certificate const &);

	/** Read a certificate from a string.
	 *  @param cert String to read.
	 *  @return remaining part of the input string after the certificate which was read.
	 */
	std::string read_string (std::string);

	/** Return the certificate as a string
	 *  @param with_begin_end true to include the -----BEGIN CERTIFICATE--- / -----END CERTIFICATE----- markers
	 *  @return Certificate string
	 */
	std::string certificate (bool with_begin_end = false) const;

	std::string serial () const;

	/** @return Certificate's issuer, in the form
	 *  dnqualifier=&lt;dnQualififer&gt;,CN=&lt;commonName&gt;,OU=&lt;organizationalUnitName&gt,O=&lt;organizationName&gt;
	 *  and with + signs escaped to \+
	 */
	std::string issuer () const;

	std::string subject () const;
	std::string subject_common_name () const;
	std::string subject_organization_name () const;
	std::string subject_organizational_unit_name () const;
	struct tm not_before () const;
	struct tm not_after () const;

	X509* x509 () const {
		return _certificate;
	}

	/** @return RSA public key from this Certificate.  Caller must not free the returned value. */
	RSA* public_key () const;

	/** @return thumbprint of the to-be-signed portion of this certificate */
	std::string thumbprint () const;

	bool has_utf8_strings () const;

private:

	static std::string name_for_xml (X509_NAME *);
	static std::string asn_to_utf8 (ASN1_STRING *);
	static std::string get_name_part (X509_NAME *, int);

	X509* _certificate = nullptr;
	mutable RSA* _public_key = nullptr;
};


bool operator== (Certificate const & a, Certificate const & b);
bool operator< (Certificate const & a, Certificate const & b);
std::ostream& operator<< (std::ostream&s, Certificate const & c);


}


#endif
