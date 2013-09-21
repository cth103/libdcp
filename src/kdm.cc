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
		throw FileError ("could not find RSA private key file", private_key);
	}
	
	RSA* rsa = PEM_read_RSAPrivateKey (private_key_file, 0, 0, 0);
	fclose (private_key_file);	
	if (!rsa) {
		throw FileError ("could not read RSA private key file", private_key);
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

KDMKey::KDMKey (shared_ptr<const Signer> signer, string cpl_id, string key_id, boost::posix_time::ptime from, boost::posix_time::ptime until, Key key)
	: _cpl_id (cpl_id)
	, _key_id (key_id),
	, _not_valid_before (ptime_to_string (from))
	, _not_valid_after (ptime_to_string (until))
	, _key (key)
{
	/* Magic value specified by SMPTE S430-1-2006 */
	_structure_id[] = { 0xf1, 0xdc, 0x12, 0x44, 0x60, 0x16, 0x9a, 0x0e, 0x85, 0xbc, 0x30, 0x06, 0x42, 0xf8, 0x66, 0xab };
	
	base64_decode (signer->certificates()->leaf()->thumbprint (), _signer_thumbprint, 20);
}

KDMKey::KDMKey (uint8_t const * raw, int len)
{
	switch (len) {
	case 134:
		/* interop */
		/* [0-15] is structure id (fixed sequence specified by standard) */
		raw += 16;
		get (_signer_thumbprint, &raw, 20);
		_cpl_id = get_uuid (&raw);
		_key_id = get_uuid (&raw);
		_not_valid_before = get (&raw, 25);
		_not_valid_after = get (&raw, 25);
		_key = Key (raw);
		break;
	case 138:
		/* SMPTE */
		/* [0-15] is structure id (fixed sequence specified by standard) */
		raw += 16;
		get (_signer_thumbprint, &raw, 20);
		_cpl_id = get_uuid (&raw);
		_key_type = get (&raw, 4);
		_key_id = get_uuid (&raw);
		_not_valid_before = get (&raw, 25);
		_not_valid_after = get (&raw, 25);
		_key = Key (raw);
		break;
	default:
		assert (false);
	}
}


string
KDMKey::base64 () const
{
	/* XXX: SMPTE only */
	uint8_t block[138];
	uint8_t* p = block;

	/* Magic value specified by SMPTE S430-1-2006 */
	uint8_t structure_id[] = { 0xf1, 0xdc, 0x12, 0x44, 0x60, 0x16, 0x9a, 0x0e, 0x85, 0xbc, 0x30, 0x06, 0x42, 0xf8, 0x66, 0xab };
	put (&p, structure_id, 16);
	put (&p, _signer_thumbprint, 20);
	put_uuid (&p, _cpl_id);
	put (&p, _key_type, 4);
	put_uuid (&p, _key_id);
	put (&p, _not_valid_before.c_str(), 25);
	put (&p, _not_valid_after.c_str(), 25);
	put (&p, _key.value(), ASDCP::KeyLen);

	/* Lazy overallocation */
	char string[138 * 2];
	return Kumu::base64encode (block, 138, string, 138 * 2);
}

string
KDMKey::get (uint8_t const ** p, int N) const
{
	string g;
	for (int i = 0; i < N; ++i) {
		g += **p;
		(*p)++;
	}

	return g;
}

void
KDMKey::get (uint8_t const * o, uint8_t const ** p, int N) const
{
	memcpy (o, *p, N);
	*p += N;
}

string
KDMKey::get_uuid (unsigned char const ** p) const
{
	stringstream g;
	
	for (int i = 0; i < 16; ++i) {
		g << setw(2) << setfill('0') << hex << static_cast<int> (**p);
		(*p)++;
		if (i == 3 || i == 5 || i == 7 || i == 9) {
			g << '-';
		}
	}

	return g.str ();
}

void
KDMKey::put (uint8_t ** d, uint8_t const * s, int N) const
{
	memcpy (*d, s, N);
	(*d) += N;
}

void
KDMKey::put_uuid (uint8_t ** d, string id) const
{
	id.erase (id.remove (id.begin(), id.end(), "-"));
	for (int i = 0; i < 32; i += 2) {
		stringstream s;
		s << id[i] << id[i + 1];
		s >> *d++;
	}
}
