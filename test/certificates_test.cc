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

BOOST_AUTO_TEST_CASE (certificates)
{
	libdcp::CertificateChain c;

	c.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate ("test/data/crypt/ca.self-signed.pem")));
	c.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate ("test/data/crypt/intermediate.signed.pem")));
	c.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate ("test/data/crypt/leaf.signed.pem")));
	
	BOOST_CHECK_EQUAL (
		c.root()->issuer(),
		"/O=example.org/OU=example.org/CN=.smpte-430-2.ROOT.NOT_FOR_PRODUCTION/dnQualifier=rTeK7x+nopFkyphflooz6p2ZM7A="
		);
	
	BOOST_CHECK_EQUAL (
		libdcp::Certificate::name_for_xml (c.root()->issuer()),
		"dnQualifier=rTeK7x\\+nopFkyphflooz6p2ZM7A=,CN=.smpte-430-2.ROOT.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);

	BOOST_CHECK_EQUAL (c.root()->serial(), "5");

	BOOST_CHECK_EQUAL (
		libdcp::Certificate::name_for_xml (c.root()->subject()),
		"dnQualifier=rTeK7x\\+nopFkyphflooz6p2ZM7A=,CN=.smpte-430-2.ROOT.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);
}
