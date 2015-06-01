/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#include <boost/test/unit_test.hpp>
#include "certificates.h"

using std::list;
using boost::shared_ptr;

BOOST_AUTO_TEST_CASE (certificates)
{
	libdcp::CertificateChain c;

	c.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate (boost::filesystem::path ("test/ref/crypt/ca.self-signed.pem"))));
	c.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate (boost::filesystem::path ("test/ref/crypt/intermediate.signed.pem"))));
	c.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate (boost::filesystem::path ("test/ref/crypt/leaf.signed.pem"))));

	list<shared_ptr<libdcp::Certificate> > leaf_to_root = c.leaf_to_root ();

	list<shared_ptr<libdcp::Certificate> >::iterator i = leaf_to_root.begin ();

	/* Leaf */
	BOOST_CHECK_EQUAL (*i, c.leaf ());
	
	BOOST_CHECK_EQUAL (
		c.leaf()->issuer(),
		"dnQualifier=bmtwThq3srgxIAeRMjX6BFhgLDw=,CN=.smpte-430-2.INTERMEDIATE.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);

	BOOST_CHECK_EQUAL (
		c.leaf()->subject(),
		"dnQualifier=d95fGDzERNdxfYPgphvAR8A18L4=,CN=CS.smpte-430-2.LEAF.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);
	
	++i;

	/* Intermediate */
	BOOST_CHECK_EQUAL (
		(*i)->issuer(),
		"dnQualifier=ndND9A/cODo2rTdrbLVmfQnoaSc=,CN=.smpte-430-2.ROOT.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);

	BOOST_CHECK_EQUAL (
		(*i)->subject(),
		"dnQualifier=bmtwThq3srgxIAeRMjX6BFhgLDw=,CN=.smpte-430-2.INTERMEDIATE.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);
	
	++i;

	/* Root */
	BOOST_CHECK_EQUAL (*i, c.root ());
	BOOST_CHECK_EQUAL (
		c.root()->issuer(),
		"dnQualifier=ndND9A/cODo2rTdrbLVmfQnoaSc=,CN=.smpte-430-2.ROOT.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);

	BOOST_CHECK_EQUAL (c.root()->serial(), "5");

	BOOST_CHECK_EQUAL (
		c.root()->subject(),
		"dnQualifier=ndND9A/cODo2rTdrbLVmfQnoaSc=,CN=.smpte-430-2.ROOT.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);

	/* Check that reconstruction from a string works */
	libdcp::Certificate test (c.root()->certificate (true));
	BOOST_CHECK_EQUAL (test.certificate(), c.root()->certificate());
}
