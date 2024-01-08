/*
    Copyright (C) 2012-2019 Carl Hetherington <cth@carlh.net>

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

#include "certificate.h"
#include "certificate_chain.h"
#include "util.h"
#include "exceptions.h"
#include "test.h"
#include <boost/test/unit_test.hpp>
#include <iostream>

using std::list;
using std::string;
using std::shared_ptr;

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
	BOOST_CHECK_EQUAL (i->thumbprint(), "EZg5wDcihccWqwdg59Y8D+IJpYM=");

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
	BOOST_CHECK_EQUAL (i->thumbprint(), "GwM6ex2UVlWclH8f1uV7W1n0EEU=");
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
	BOOST_CHECK_EQUAL (i->thumbprint(), "zU8NVNwI2PYejmSYRntG7c6sdTw=");
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

	{
		/* This is a chain, not an individual certificate, so it should throw an exception */
		BOOST_CHECK_THROW (dcp::Certificate (dcp::file_to_string (private_test / "chain.pem")), dcp::MiscError);
	}

	BOOST_CHECK_THROW (dcp::Certificate (dcp::file_to_string (private_test / "no-begin.pem")), dcp::MiscError);
	BOOST_CHECK_THROW (dcp::Certificate ("foo"), dcp::MiscError);
}

/** Check that dcp::CertificateChain::chain_valid() and ::root_to_leaf() basically work */
BOOST_AUTO_TEST_CASE (certificates_validation1)
{
	dcp::CertificateChain good;
	good.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	good.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	good.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	BOOST_CHECK (good.chain_valid(good._certificates));
}

/** Check that dcp::CertificateChain::chain_valid() and ::root_to_leaf() basically work */
BOOST_AUTO_TEST_CASE (certificates_validation2)
{
	dcp::CertificateChain good;
	good.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	BOOST_CHECK (good.chain_valid(good._certificates));
}

/** Check that dcp::CertificateChain::chain_valid() and ::root_to_leaf() basically work */
BOOST_AUTO_TEST_CASE (certificates_validation3)
{
	dcp::CertificateChain bad;
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	BOOST_CHECK (!bad.chain_valid(bad._certificates));
	BOOST_CHECK_THROW (bad.root_to_leaf(), dcp::CertificateChainError);
}

/** Check that dcp::CertificateChain::chain_valid() and ::root_to_leaf() basically work */
BOOST_AUTO_TEST_CASE (certificates_validation4)
{
	dcp::CertificateChain bad;
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	BOOST_CHECK (!bad.chain_valid(bad._certificates));
	BOOST_CHECK_NO_THROW (bad.root_to_leaf());
}

/** Check that dcp::CertificateChain::chain_valid() and ::root_to_leaf() basically work */
BOOST_AUTO_TEST_CASE (certificates_validation5)
{
	dcp::CertificateChain bad;
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	BOOST_CHECK (!bad.chain_valid(bad._certificates));
	BOOST_CHECK_NO_THROW (bad.root_to_leaf());
}

/** Check that dcp::CertificateChain::chain_valid() and ::root_to_leaf() basically work */
BOOST_AUTO_TEST_CASE (certificates_validation6)
{
	dcp::CertificateChain bad;
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	BOOST_CHECK (!bad.chain_valid(bad._certificates));
	BOOST_CHECK_NO_THROW (bad.root_to_leaf());
}

/** Check that dcp::CertificateChain::chain_valid() and ::root_to_leaf() basically work */
BOOST_AUTO_TEST_CASE (certificates_validation7)
{
	dcp::CertificateChain bad;
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem")));
	BOOST_CHECK (!bad.chain_valid(bad._certificates));
	BOOST_CHECK_THROW (bad.root_to_leaf(), dcp::CertificateChainError);
}

/** Check that dcp::CertificateChain::chain_valid() and ::root_to_leaf() basically work */
BOOST_AUTO_TEST_CASE (certificates_validation8)
{
	dcp::CertificateChain bad;
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/intermediate.signed.pem")));
	bad.add (dcp::Certificate (dcp::file_to_string ("test/ref/crypt/ca.self-signed.pem")));
	BOOST_CHECK (!bad.chain_valid(bad._certificates));
	BOOST_CHECK_THROW (bad.root_to_leaf(), dcp::CertificateChainError);
}

/** Check that we can create a valid chain */
BOOST_AUTO_TEST_CASE (certificates_validation9)
{
	dcp::CertificateChain good (
		boost::filesystem::path ("openssl"),
		10 * 365,
		"dcpomatic.com",
		"dcpomatic.com",
		".dcpomatic.smpte-430-2.ROOT",
		".dcpomatic.smpte-430-2.INTERMEDIATE",
		"CS.dcpomatic.smpte-430-2.LEAF"
		);

	BOOST_CHECK_NO_THROW (good.root_to_leaf());
}

/** Check that we can create a valid chain */
BOOST_AUTO_TEST_CASE (certificates_validation10)
{
	dcp::CertificateChain good (boost::filesystem::path ("openssl"), 10 * 365);
	BOOST_CHECK_NO_THROW (good.root_to_leaf());
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
	dcp::CertificateChain another_chain (boost::filesystem::path ("openssl"), 10 * 365);
	chain.set_key (another_chain.key().get ());
	BOOST_CHECK (!chain.valid ());
}

/** Check reading of a certificate chain from a string */
BOOST_AUTO_TEST_CASE (certificate_chain_from_string)
{
	dcp::CertificateChain a (dcp::file_to_string (private_test / "chain.pem"));
	BOOST_CHECK_EQUAL (a.root_to_leaf().size(), 3U);

	dcp::CertificateChain b (dcp::file_to_string ("test/ref/crypt/leaf.signed.pem"));
	BOOST_CHECK_EQUAL (b.root_to_leaf().size(), 1U);
}

/** Check not_before and not_after */
BOOST_AUTO_TEST_CASE (certificate_not_before_after)
{
	dcp::Certificate c (dcp::file_to_string("test/ref/crypt/ca.self-signed.pem"));
	auto not_before = c.not_before();
	BOOST_CHECK_EQUAL (not_before.second(), 8);
	BOOST_CHECK_EQUAL (not_before.minute(), 20);
	BOOST_CHECK_EQUAL (not_before.hour(), 13);
	BOOST_CHECK_EQUAL (not_before.day(), 5);
	BOOST_CHECK_EQUAL (not_before.month(), 6);
	BOOST_CHECK_EQUAL (not_before.year(), 2015);
	auto not_after = c.not_after();
	BOOST_CHECK_EQUAL (not_after.second(), 8);
	BOOST_CHECK_EQUAL (not_after.minute(), 20);
	BOOST_CHECK_EQUAL (not_after.hour(), 13);
	BOOST_CHECK_EQUAL (not_after.day(), 2);
	BOOST_CHECK_EQUAL (not_after.month(), 6);
	BOOST_CHECK_EQUAL (not_after.year(), 2025);
}


/** Check for correct escaping of public key digests */
BOOST_AUTO_TEST_CASE(certificate_public_key_digest)
{
	BOOST_CHECK_EQUAL(dcp::public_key_digest("test/data/private.key"), "MekIXGBkYdh28siMnnF\\/Zs2JeK8=");
	BOOST_CHECK_EQUAL(dcp::public_key_digest("test/data/private2.key"), "dfjStQNFTdVpfzgmxQCb3x\\+y2SY=");
}


/** Create some certificates and check that the dnQualifier read from the header is always what is should be;
 *  previously it would not be if the digest contained \ or + (DoM #2716).
 */
BOOST_AUTO_TEST_CASE(certificate_dn_qualifiers)
{
	for (auto i = 0; i < 50; ++i) {
		dcp::CertificateChain chain(boost::filesystem::path("openssl"), 10 * 365);
		for (auto cert: chain.unordered()) {
			BOOST_CHECK_EQUAL(dcp::escape_digest(cert.subject_dn_qualifier()), dcp::public_key_digest(cert.public_key()));
		}
	}
}

