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
#include <boost/algorithm/string.hpp>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <libcxml/cxml.h>
#include "util.h"
#include "kdm.h"
#include "compose.hpp"
#include "exceptions.h"

using std::list;
using std::string;
using std::stringstream;
using std::hex;
using std::setw;
using std::setfill;
using std::cout;
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

	cxml::Document f ("DCinemaSecurityMessage");
	f.read_file (kdm.string ());

	shared_ptr<cxml::Node> authenticated_private = f.node_child ("AuthenticatedPrivate");
	list<shared_ptr<cxml::Node> > encrypted_keys = authenticated_private->node_children ("EncryptedKey");

	for (list<shared_ptr<cxml::Node> >::iterator i = encrypted_keys.begin(); i != encrypted_keys.end(); ++i) {

		/* Get the base-64-encoded cipher value from the KDM */
		shared_ptr<cxml::Node> cipher_data = (*i)->node_child ("CipherData");
		shared_ptr<cxml::Node> cipher_value_base64 = cipher_data->node_child ("CipherValue");

		/* Decode it from base-64 */
		unsigned char cipher_value[256];
		int const cipher_value_len = base64_decode (cipher_value_base64->content(), cipher_value, sizeof (cipher_value));

		/* Decrypt it */
		unsigned char* decrypted = new unsigned char[RSA_size(rsa)];
		int const decrypted_len = RSA_private_decrypt (cipher_value_len, cipher_value, decrypted, rsa, RSA_PKCS1_OAEP_PADDING);
		if (decrypted_len == -1) {
			delete[] decrypted;
			throw MiscError (String::compose ("Could not decrypt KDM (%1)", ERR_error_string (ERR_get_error(), 0)));
		}

		_keys.push_back (KDMKey (decrypted, decrypted_len));
		delete[] decrypted;
	}

	RSA_free (rsa);
}


KDMKey::KDMKey (unsigned char const * raw, int len)
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
		_key = Key (raw);
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
		_key = Key (raw);
		break;
	default:
		assert (false);
	}
}

string
KDMKey::get (unsigned char const ** p, int N) const
{
	string g;
	for (int i = 0; i < N; ++i) {
		g += **p;
		(*p)++;
	}

	return g;
}

string
KDMKey::get_uuid (unsigned char const ** p, int N) const
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
