/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

#include <iomanip>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <libcxml/cxml.h>
#include "KM_util.h"
#include "kdm.h"
#include "exceptions.h"

using std::list;
using std::string;
using std::stringstream;
using std::hex;
using std::setw;
using std::setfill;
using boost::shared_ptr;
using namespace libdcp;

KDM::KDM (boost::filesystem::path kdm, boost::filesystem::path private_key)
{
	/* Read the private key */
	   
	FILE* private_key_file = fopen (private_key.string().c_str(), "r");
	if (!private_key_file) {
		throw FileError ("could not find RSA private key file", private_key.string ());
	}
	
	RSA* rsa = PEM_read_RSAPrivateKey (private_key_file, 0, 0, 0);
	fclose (private_key_file);	
	if (!rsa) {
		throw FileError ("could not read RSA private key file", private_key.string ());
	}

	
	/* Read the KDM, decrypting it */

	cxml::File f (kdm.string (), "DCinemaSecurityMessage");

	shared_ptr<cxml::Node> authenticated_private = f.node_child ("AuthenticatedPrivate");
	list<shared_ptr<cxml::Node> > encrypted_keys = authenticated_private->node_children ("EncryptedKey");

	for (list<shared_ptr<cxml::Node> >::iterator i = encrypted_keys.begin(); i != encrypted_keys.end(); ++i) {

		/* Get the base-64-encoded cipher value from the KDM */
		shared_ptr<cxml::Node> cipher_data = (*i)->node_child ("CipherData");
		shared_ptr<cxml::Node> cipher_value_base64 = cipher_data->node_child ("CipherValue");

		/* Decode it from base-64 */
		unsigned char cipher_value[256];
		ui32_t cipher_value_len;
		if (Kumu::base64decode (cipher_value_base64->content().c_str(), cipher_value, sizeof (cipher_value), &cipher_value_len)) {
			RSA_free (rsa);
			throw MiscError ("could not base-64-decode CipherValue from KDM");
		}

		/* Decrypt it */
		unsigned char decrypted[2048];
		unsigned int const decrypted_len = RSA_private_decrypt (cipher_value_len, cipher_value, decrypted, rsa, RSA_PKCS1_OAEP_PADDING);
		assert (decrypted_len < sizeof (decrypted));

		_ciphers.push_back (KDMCipher (decrypted, decrypted_len));
	}

	RSA_free (rsa);
}


KDMCipher::KDMCipher (unsigned char const * raw, int len)
{
	switch (len) {
	case 134:
		/* interop */
		_structure_id = get (&raw, 16);
		_signer_thumbprint = get (&raw, 20);
		_cpl_id = get_uuid (&raw, 16);
		_key_id = get_uuid (&raw, 16);
		_not_valid_before = get (&raw, 25);
		_not_valid_after = get (&raw, 25);
		memcpy (_key_raw, raw, 16);
		_key_string = get_hex (&raw, 16);
		break;
	case 138:
		/* SMPTE */
		_structure_id = get (&raw, 16);
		_signer_thumbprint = get (&raw, 20);
		_cpl_id = get_uuid (&raw, 16);
		_key_type = get (&raw, 4);
		_key_id = get_uuid (&raw, 16);
		_not_valid_before = get (&raw, 25);
		_not_valid_after = get (&raw, 25);
		memcpy (_key_raw, raw, 16);
		_key_string = get_hex (&raw, 16);
		break;
	default:
		assert (false);
	}
}

string
KDMCipher::get (unsigned char const ** p, int N) const
{
	string g;
	for (int i = 0; i < N; ++i) {
		g += **p;
		(*p)++;
	}

	return g;
}

string
KDMCipher::get_uuid (unsigned char const ** p, int N) const
{
	stringstream g;
	
	for (int i = 0; i < N; ++i) {
		g << setw(2) << setfill('0') << hex << static_cast<int> (**p);
		(*p)++;
		if (i == 3 || i == 5 || i == 7 || i == 9) {
			g << '-';
		}
	}

	return g.str ();
}

string
KDMCipher::get_hex (unsigned char const ** p, int N) const
{
	stringstream g;
	
	for (int i = 0; i < N; ++i) {
		g << setw(2) << setfill('0') << hex << static_cast<int> (**p);
		(*p)++;
	}

	return g.str ();
}
