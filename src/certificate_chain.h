/*
    Copyright (C) 2013-2016 Carl Hetherington <cth@carlh.net>

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

/** @file  src/signer_chain.h
 *  @brief Functions to make signer chains.
 */

#ifndef LIBDCP_CERTIFICATE_CHAIN_H
#define LIBDCP_CERTIFICATE_CHAIN_H

#include "certificate.h"
#include "types.h"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

namespace xmlpp {
	class Node;
}

struct certificates_validation1;
struct certificates_validation2;
struct certificates_validation3;
struct certificates_validation4;
struct certificates_validation5;
struct certificates_validation6;
struct certificates_validation7;
struct certificates_validation8;

namespace dcp {

/** @class CertificateChain
 *  @brief A chain of any number of certificates, from root to leaf.
 */
class CertificateChain
{
public:
	CertificateChain () {}

	/** Create a chain of certificates for signing things.
	 *  @param openssl Name of openssl binary (if it is on the path) or full path.
	 *  @return Directory (which should be deleted by the caller) containing:
	 *    - ca.self-signed.pem      self-signed root certificate
	 *    - intermediate.signed.pem intermediate certificate
	 *    - leaf.key                leaf certificate private key
	 *    - leaf.signed.pem         leaf certificate
	 */
	CertificateChain (
		boost::filesystem::path openssl,
		std::string organisation = "example.org",
		std::string organisational_unit = "example.org",
		std::string root_common_name = ".smpte-430-2.ROOT.NOT_FOR_PRODUCTION",
		std::string intermediate_common_name = ".smpte-430-2.INTERMEDIATE.NOT_FOR_PRODUCTION",
		std::string leaf_common_name = "CS.smpte-430-2.LEAF.NOT_FOR_PRODUCTION"
		);

	explicit CertificateChain (std::string);

	void add (Certificate c);
	void remove (Certificate c);
	void remove (int);

	Certificate root () const;
	Certificate leaf () const;

	typedef std::list<Certificate> List;

	List leaf_to_root () const;
	List root_to_leaf () const;
	List unordered () const;

	bool valid (std::string* reason = 0) const;
	bool chain_valid () const;
	bool private_key_valid () const;

	void sign (xmlpp::Element* parent, Standard standard) const;
	void add_signature_value (xmlpp::Node* parent, std::string ns) const;

	boost::optional<std::string> key () const {
		return _key;
	}

	void set_key (std::string k) {
		_key = k;
	}

	std::string chain () const;

private:
	friend struct ::certificates;
	friend struct ::certificates_validation1;
	friend struct ::certificates_validation2;
	friend struct ::certificates_validation3;
	friend struct ::certificates_validation4;
	friend struct ::certificates_validation5;
	friend struct ::certificates_validation6;
	friend struct ::certificates_validation7;
	friend struct ::certificates_validation8;

	bool chain_valid (List const & chain) const;

	/** Our certificates, not in any particular order */
	List _certificates;
	/** Leaf certificate's private key, if known */
	boost::optional<std::string> _key;
};

}

#endif
