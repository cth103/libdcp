/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_SIGNER_H
#define LIBDCP_SIGNER_H

/** @file  src/signer.h
 *  @brief Signer class.
 */

#include "certificate.h"
#include "certificate_chain.h"
#include "types.h"
#include <boost/filesystem.hpp>

namespace xmlpp {
	class Element;
	class Node;
}

namespace dcp {

/** @class Signer
 *  @brief A class which can sign XML files.
 */
class Signer
{
public:
	Signer (boost::filesystem::path openssl);

	Signer (
		boost::filesystem::path openssl,
		std::string organisation,
		std::string organisational_unit,
		std::string root_common_name,
		std::string intermediate_common_name,
		std::string leaf_common_name
		);

	/** @param c Certificate chain to sign with.
	 *  @param k Key to sign with as a PEM-format string.
	 */
	Signer (CertificateChain c, std::string k)
		: _certificates (c)
		, _key (k)
	{}

	void sign (xmlpp::Element* parent, Standard standard) const;
	void add_signature_value (xmlpp::Node* parent, std::string ns) const;

	CertificateChain const & certificates () const {
		return _certificates;
	}

	CertificateChain& certificates () {
		return _certificates;
	}

	std::string key () const {
		return _key;
	}

	void set_key (std::string k) {
		_key = k;
	}

	bool valid () const;

private:
	void create (boost::filesystem::path directory);

	/** Certificate chain to sign with */
	CertificateChain _certificates;
	/** Key to sign with as a PEM-format string */
	std::string _key;
};

}

#endif
