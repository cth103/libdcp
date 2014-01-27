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

/** @file  src/signer.h
 *  @brief Signer class.
 */

#include "certificates.h"
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
	/** @param c Certificate chain to sign with.
	 *  @param k Key to sign with.
	 */
	Signer (CertificateChain c, boost::filesystem::path k)
		: _certificates (c)
		, _key (k)
	{}

	void sign (xmlpp::Element* parent, Standard standard) const;
	void add_signature_value (xmlpp::Node* parent, std::string ns) const;

	CertificateChain const & certificates () const {
		return _certificates;
	}
	
private:	

	/** Certificate chain to sign with */
	CertificateChain _certificates;
	/** Filename of signer key */
	boost::filesystem::path _key;
};

}
