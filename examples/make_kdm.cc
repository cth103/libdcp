/*
    Copyright (C) 2012-2022 Carl Hetherington <cth@carlh.net>

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
*/


/* If you are using an installed libdcp, these #includes would need to be changed to
#include <dcp/decrypted_kdm.h>
... etc. ...
*/

#include "certificate_chain.h"
#include "decrypted_kdm.h"
#include "encrypted_kdm.h"
#include "key.h"
#include "util.h"


constexpr char recipient_certificate[] = "-----BEGIN CERTIFICATE-----\n"
"MIIEaTCCA1GgAwIBAgIBBzANBgkqhkiG9w0BAQsFADCBhTEWMBQGA1UEChMNZGNw\n"
"b21hdGljLmNvbTEWMBQGA1UECxMNZGNwb21hdGljLmNvbTEsMCoGA1UEAxMjLmRj\n"
"cG9tYXRpYy5zbXB0ZS00MzAtMi5JTlRFUk1FRElBVEUxJTAjBgNVBC4THEJyRE1x\n"
"TjF4bytQcy9ZZTdLTmVhNzRHdlI5Yz0wHhcNMjIwOTIwMTk1MTQxWhcNMzIwOTIy\n"
"MTk1MTQxWjB/MRYwFAYDVQQKEw1kY3BvbWF0aWMuY29tMRYwFAYDVQQLEw1kY3Bv\n"
"bWF0aWMuY29tMSYwJAYDVQQDEx1DUy5kY3BvbWF0aWMuc21wdGUtNDMwLTIuTEVB\n"
"RjElMCMGA1UELhMcSmR4aEVZdURUR05RQlh2TFpsWEZReVVGSzdZPTCCASIwDQYJ\n"
"KoZIhvcNAQEBBQADggEPADCCAQoCggEBAOWq+41uCQbcwQ8+Sh3kVUiG7b9SjU5k\n"
"L8my4IEW2ajjUSDff/a2AM7W+BBAzuAWXpZe2+x+/UdAKOIBLFuyWFKbKLMgh0i0\n"
"WuukOqeEdr+ZD09PgvHriEk9pXcYDhGxp3OmLVR7kmK0mn+SwLfNZ2LUGJSItGra\n"
"ciOPcJgbj/2jyqIkFOz6oZk4xPNdhhM1q41ledTQY/DjesoQqCVZv+lJlAOhc7Sy\n"
"vynk6WXF+PtRYjTqMFuHKAjZaNjKBFu60gYp3xVdmAyOmD/7DHFtum9HgTr0GM9l\n"
"NfBuU7tFjwl7uylB8/Eff2OLo1cSOH+O2uvzaat1ceYETlCLDeyneY8CAwEAAaOB\n"
"6DCB5TAMBgNVHRMBAf8EAjAAMAsGA1UdDwQEAwIFoDAdBgNVHQ4EFgQUJdxhEYuD\n"
"TGNQBXvLZlXFQyUFK7YwgagGA1UdIwSBoDCBnYAUBrDMqN1xo+Ps/Ye7KNea74Gv\n"
"R9ehgYGkfzB9MRYwFAYDVQQKEw1kY3BvbWF0aWMuY29tMRYwFAYDVQQLEw1kY3Bv\n"
"bWF0aWMuY29tMSQwIgYDVQQDExsuZGNwb21hdGljLnNtcHRlLTQzMC0yLlJPT1Qx\n"
"JTAjBgNVBC4THFc4YnBZTXkyVlF2WllDcGhOVWRqUVhLcGVNYz2CAQYwDQYJKoZI\n"
"hvcNAQELBQADggEBAHNocvxiWHwh0JKgf5cS1x7NHjnL9V5NSKRFH6qKZkSEWOdk\n"
"05+n99zxDzjh600DAAp8QIQ8FgC93TXsBg/owrKyZhVpDaRt5ZmUaLmmJUFBtEkJ\n"
"qmlXmZGu213zTCT1coMFNXiEImhUt/vd5JOmNsGydCyzEipr7vt8aDr/xCCJdcUo\n"
"y2Q5MfrD5wC4PgPBampSsbIu6IrTfx5kbrKIg/4X2VGFzyNDHz8N4+wfPGBuo4Ra\n"
"6YWAd58LUb1Wp7dP27HkQH74QRPvrVNOC4vcjnHnBtlWmFzGOi+1e4stWupL7IYd\n"
"Apivqyi9TqCUHkjLyuZPjEU30borxqrl918Z/Co=\n"
"-----END CERTIFICATE-----\n";


int main()
{
	/* The parameter to this call specifies where resources can be found, i.e.
	 * the tags and schema directories.
	 */
	dcp::init(boost::filesystem::path("."));

	/* Make a KDM to hold one or more asset keys */
	dcp::DecryptedKDM decrypted_kdm(
		// valid from time
		dcp::LocalTime("2023-01-20T09:30:00"),
		// valid to time
		dcp::LocalTime("2023-11-01T09:30:00"),
		// annotation text
		"KDM annotation",
		// content title text
		"KDM content title",
		// issue date; using dcp::LocalTime() gives the time when the code is run
		dcp::LocalTime().as_string()
		);

	/* Add a key which can decrypt an asset; you should do this for each encrypted asset
	 * that the KDM must unlock.
	 */
	decrypted_kdm.add_key(
		// key type (MDIK is for image MXFs)
		std::string{"MDIK"},
		// key ID
		"01234567-89ab-cdef-0123-456789abcdef",
		// key
		dcp::Key("00112233445566778899aabbccddeeff"),
		// CPL ID
		"fedcba98-7654-3210-fedc-ba9876543210",
		// Standard for KDM (interop/SMPTE)
		dcp::Standard::SMPTE
		);

	/* Make a certificate chain to sign the KDM */
	auto signer = std::make_shared<dcp::CertificateChain>("/usr/bin/openssl", 365);

	/* Certificate of the recipient projector/media block */
	dcp::Certificate recipient(recipient_certificate);

	/* Encrypt the KDM */
	auto encrypted_kdm = decrypted_kdm.encrypt(
		signer,
		recipient,
		{},
		dcp::Formulation::MODIFIED_TRANSITIONAL_1,
		false,
		{}
		);

	/* Dump the XML to the console */
	std::cout << encrypted_kdm.as_xml();

	return 0;
}

