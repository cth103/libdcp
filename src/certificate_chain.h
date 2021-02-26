/*
    Copyright (C) 2013-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/certificate_chain.h
 *  @brief CertificateChain class
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
 *
 *  A CertificateChain object can also (optionally) hold the private key corresponding
 *  to the leaf certificate.
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

	/** Read a CertificateChain from a string.
	 *  @param s A string containing one or more PEM-encoded certificates.
	 */
	explicit CertificateChain (std::string s);

	/** Add a certificate to the chain.
	 *  @param c Certificate to add.
	 */
	void add (Certificate c);

	/** Remove a certificate from the chain.
	 *  @param c Certificate to remove.
	 */
	void remove (Certificate c);

	/** Remove the i'th certificate in the chain, as listed
	 *  from root to leaf.
	 */
	void remove (int i);

	/** @return Root certificate */
	Certificate root () const;

	/** @return Leaf certificate */
	Certificate leaf () const;

	typedef std::vector<Certificate> List;

	/** @return Certificates in order from leaf to root */
	List leaf_to_root () const;
	/** @return Certificates in order from root to leaf */
	List root_to_leaf () const;
	List unordered () const;

	bool valid (std::string* reason = nullptr) const;

	/** Check to see if the chain is valid (i.e. root signs the intermediate, intermediate
	 *  signs the leaf and so on) and that the private key (if there is one) matches the
	 *  leaf certificate.
	 *  @return true if it's ok, false if not.
	 */
	bool chain_valid () const;

	/** Check that there is a valid private key for the leaf certificate.
	 *  Will return true if there are no certificates.
	 */
	bool private_key_valid () const;

	/** Add a &lt;Signer&gt; and &lt;ds:Signature&gt; nodes to an XML node.
	 *  @param parent XML node to add to.
	 *  @param standard INTEROP or SMPTE.
	 */
	void sign (xmlpp::Element* parent, Standard standard) const;

	/** Sign an XML node.
	 *
	 *  @param parent Node to sign.
	 *  @param ns Namespace to use for the signature XML nodes.
	 */
	void add_signature_value (xmlpp::Element* parent, std::string ns, bool add_indentation) const;

	boost::optional<std::string> key () const {
		return _key;
	}

	void set_key (std::string k) {
		_key = k;
	}

	std::string chain () const;

private:
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
	/** Leaf certificate's private key, if known, in PEM format */
	boost::optional<std::string> _key;
};


}


#endif
