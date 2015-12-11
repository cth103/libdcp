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
#include "certificate.h"
#include "certificate_chain.h"
#include "util.h"
#include "exceptions.h"
#include "test.h"

using std::list;
using std::string;
using boost::shared_ptr;

/** Check that loading certificates from files via strings works */
BOOST_AUTO_TEST_CASE (certificates1)
{
	dcp::CertificateChain c;

	c.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	c.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	c.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));

	dcp::CertificateChain::List leaf_to_root = c.leaf_to_root ();

	dcp::CertificateChain::List::iterator i = leaf_to_root.begin ();

	/* Leaf */
	BOOST_CHECK_EQUAL (*i, c.leaf ());

	BOOST_CHECK_EQUAL (
		c.leaf().issuer(),
		"dnQualifier=6eat8r33US71avuQEojmH\\+bjk84=,CN=.smpte-430-2.INTERMEDIATE.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);

	BOOST_CHECK_EQUAL (
		c.leaf().subject(),
		"dnQualifier=QFVlym7fuql6bPOnY38aaO1ZPW4=,CN=CS.smpte-430-2.LEAF.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);

	++i;

	/* Intermediate */
	BOOST_CHECK_EQUAL (
		i->issuer(),
		"dnQualifier=DCnRdHFbcv4ANVUq2\\+wMVALFSec=,CN=.smpte-430-2.ROOT.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);

	BOOST_CHECK_EQUAL (
		i->subject(),
		"dnQualifier=6eat8r33US71avuQEojmH\\+bjk84=,CN=.smpte-430-2.INTERMEDIATE.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);

	++i;

	/* Root */
	BOOST_CHECK_EQUAL (*i, c.root ());
	BOOST_CHECK_EQUAL (
		c.root().issuer(),
		"dnQualifier=DCnRdHFbcv4ANVUq2\\+wMVALFSec=,CN=.smpte-430-2.ROOT.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);

	BOOST_CHECK_EQUAL (c.root().serial(), "5");

	BOOST_CHECK_EQUAL (
		c.root().subject(),
		"dnQualifier=DCnRdHFbcv4ANVUq2\\+wMVALFSec=,CN=.smpte-430-2.ROOT.NOT_FOR_PRODUCTION,OU=example.org,O=example.org"
		);

	/* Check that reconstruction from a string works */
	dcp::Certificate test (c.root().certificate (true));
	BOOST_CHECK_EQUAL (test.certificate(), c.root().certificate());
}

/** Check some more certificate-from-strings */
BOOST_AUTO_TEST_CASE (certificates2)
{
	{
		dcp::Certificate c (dcp::file_to_string (private_test / "CA.GDC-TECH.COM_SA2100_A14903.crt.crt"));
		BOOST_CHECK_EQUAL (c.certificate(true), dcp::file_to_string (private_test / "CA.GDC-TECH.COM_SA2100_A14903.crt.crt.reformatted"));
	}

	{
		dcp::Certificate c (dcp::file_to_string (private_test / "usl-cert.pem"));
		BOOST_CHECK_EQUAL (c.certificate(true), dcp::file_to_string (private_test / "usl-cert.pem.trimmed"));
	}

	BOOST_CHECK_THROW (dcp::Certificate (dcp::file_to_string (private_test / "no-begin.pem")), dcp::MiscError);
	BOOST_CHECK_THROW (dcp::Certificate ("foo"), dcp::MiscError);
}

/** Check that dcp::CertificateChain::valid() and ::attempt_reorder() basically work */
BOOST_AUTO_TEST_CASE (certificates_validation)
{
	dcp::CertificateChain good1;
	good1.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	good1.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	good1.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	BOOST_CHECK (good1.valid ());

	dcp::CertificateChain good2;
	good2.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	BOOST_CHECK (good2.valid ());

	dcp::CertificateChain bad1;
	bad1.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	bad1.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	BOOST_CHECK (!bad1.valid ());
	BOOST_CHECK (!bad1.attempt_reorder ());

	dcp::CertificateChain bad2;
	bad2.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	bad2.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	bad2.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	BOOST_CHECK (!bad2.valid ());
	BOOST_CHECK (bad2.attempt_reorder ());

	dcp::CertificateChain bad3;
	bad3.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	bad3.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	bad3.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	BOOST_CHECK (!bad3.valid ());
	BOOST_CHECK (bad3.attempt_reorder ());

	dcp::CertificateChain bad4;
	bad4.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	bad4.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	bad4.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	BOOST_CHECK (!bad4.valid ());
	BOOST_CHECK (bad4.attempt_reorder ());

	dcp::CertificateChain bad5;
	bad5.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	bad5.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	BOOST_CHECK (!bad5.valid ());
	BOOST_CHECK (!bad5.attempt_reorder ());
}

/** Check that dcp::Signer::valid() basically works */
BOOST_AUTO_TEST_CASE (signer_validation)
{
	/* Check a valid signer */
	dcp::CertificateChain chain;
	chain.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	chain.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	chain.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	chain.set_key (dcp::file_to_string ("test/ref/crypt/leaf.key"));
	BOOST_CHECK (chain.valid ());

	/* Put in an unrelated key and the signer should no longer be valid */
	dcp::CertificateChain another_chain ("openssl");
	chain.set_key (another_chain.key().get ());
	BOOST_CHECK (!chain.valid ());
}
