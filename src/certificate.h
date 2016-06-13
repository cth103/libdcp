/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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
 *  @brief Certificate class.
 */

#ifndef LIBDCP_CERTIFICATE_H
#define LIBDCP_CERTIFICATE_H

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
		, _extra_data (false)
	{}

	explicit Certificate (std::string);
	explicit Certificate (X509 *);
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

	bool extra_data () const {
		return _extra_data;
	}

private:
	bool read_string (std::string);

	static std::string name_for_xml (X509_NAME *);
	static std::string asn_to_utf8 (ASN1_STRING *);
	static std::string get_name_part (X509_NAME *, int);

	X509* _certificate;
	mutable RSA* _public_key;
	/** true if extra data was found when this certificate was read
	    from a string.
	*/
	bool _extra_data;
};

bool operator== (Certificate const & a, Certificate const & b);
bool operator< (Certificate const & a, Certificate const & b);
std::ostream& operator<< (std::ostream&s, Certificate const & c);

}

#endif
