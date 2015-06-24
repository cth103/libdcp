/*
    Copyright (C) 2013-2015 Carl Hetherington <cth@carlh.net>

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

#include "decrypted_kdm.h"
#include "decrypted_kdm_key.h"
#include "encrypted_kdm.h"
#include "reel_mxf.h"
#include "reel_asset.h"
#include "util.h"
#include "exceptions.h"
#include "cpl.h"
#include "signer.h"
#include "dcp_assert.h"
#include "AS_DCP.h"
#include "KM_util.h"
#include "compose.hpp"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <boost/foreach.hpp>

using std::list;
using std::string;
using std::stringstream;
using std::setw;
using std::setfill;
using std::hex;
using std::pair;
using boost::shared_ptr;
using namespace dcp;

static void
put (uint8_t ** d, string s)
{
        memcpy (*d, s.c_str(), s.length());
        (*d) += s.length();
}

static void
put (uint8_t ** d, uint8_t const * s, int N)
{
        memcpy (*d, s, N);
        (*d) += N;
}

static void
put_uuid (uint8_t ** d, string id)
{
        id.erase (std::remove (id.begin(), id.end(), '-'));
        for (int i = 0; i < 32; i += 2) {
                stringstream s;
                s << id[i] << id[i + 1];
                int h;
                s >> hex >> h;
                **d = h;
                (*d)++;
        }
}

static string
get_uuid (unsigned char ** p)
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

static string
get (uint8_t ** p, int N)
{
	string g;
	for (int i = 0; i < N; ++i) {
		g += **p;
		(*p)++;
	}

	return g;
}

DecryptedKDM::DecryptedKDM (EncryptedKDM const & kdm, string private_key)
{
	/* Read the private key */

	BIO* bio = BIO_new_mem_buf (const_cast<char *> (private_key.c_str ()), -1);
	if (!bio) {
		throw MiscError ("could not create memory BIO");
	}

	RSA* rsa = PEM_read_bio_RSAPrivateKey (bio, 0, 0, 0);
	if (!rsa) {
		throw FileError ("could not read RSA private key file", private_key, errno);
	}

	/* Use the private key to decrypt the keys */

	BOOST_FOREACH (string const & i, kdm.keys ()) {
		/* Decode the base-64-encoded cipher value from the KDM */
		unsigned char cipher_value[256];
		int const cipher_value_len = base64_decode (i, cipher_value, sizeof (cipher_value));

		/* Decrypt it */
		unsigned char * decrypted = new unsigned char[RSA_size(rsa)];
		int const decrypted_len = RSA_private_decrypt (cipher_value_len, cipher_value, decrypted, rsa, RSA_PKCS1_OAEP_PADDING);
		if (decrypted_len == -1) {
			delete[] decrypted;
			throw MiscError (String::compose ("Could not decrypt KDM (%1)", ERR_error_string (ERR_get_error(), 0)));
		}

		unsigned char* p = decrypted;
		switch (decrypted_len) {
		case 134:
		{
			/* Inter-op */
			/* 0 is structure id (fixed sequence specified by standard) [16 bytes] */
			p += 16;
			/* 16 is is signer thumbprint [20 bytes] */
			p += 20;
			/* 36 is CPL id [16 bytes] */
			string const cpl_id = get_uuid (&p);
			/* 52 is key id [16 bytes] */
			string const key_id = get_uuid (&p);
			/* 68 is not-valid-before (a string) [25 bytes] */
			p += 25;
			/* 93 is not-valid-after (a string) [25 bytes] */
			p += 25;
			/* 118 is the key [ASDCP::KeyLen bytes] */
			_keys.push_back (DecryptedKDMKey ("", key_id, Key (p), cpl_id));
			break;
		}
		case 138:
		{
			/* SMPTE */
			/* 0 is structure id (fixed sequence specified by standard) [16 bytes] */
			p += 16;
			/* 16 is is signer thumbprint [20 bytes] */
			p += 20;
			/* 36 is CPL id [16 bytes] */
			string const cpl_id = get_uuid (&p);
			/* 52 is key type [4 bytes] */
			string const key_type = get (&p, 4);
			/* 56 is key id [16 bytes] */
			string const key_id = get_uuid (&p);
			/* 72 is not-valid-before (a string) [25 bytes] */
			p += 25;
			/* 97 is not-valid-after (a string) [25 bytes] */
			p += 25;
			/* 112 is the key [ASDCP::KeyLen bytes] */
			_keys.push_back (DecryptedKDMKey (key_type, key_id, Key (p), cpl_id));
			break;
		}
		default:
			DCP_ASSERT (false);
		}

		delete[] decrypted;
	}

	RSA_free (rsa);
	BIO_free (bio);
}

DecryptedKDM::DecryptedKDM (
	boost::shared_ptr<const CPL> cpl,
	Key key,
	LocalTime not_valid_before,
	LocalTime not_valid_after,
	string annotation_text,
	string content_title_text,
	string issue_date
	)
	: _not_valid_before (not_valid_before)
	, _not_valid_after (not_valid_after)
	, _annotation_text (annotation_text)
	, _content_title_text (content_title_text)
	, _issue_date (issue_date)
{
	/* Create DecryptedKDMKey objects for each encryptable asset */
	BOOST_FOREACH(shared_ptr<const ReelAsset> i, cpl->reel_assets ()) {
		shared_ptr<const ReelMXF> mxf = boost::dynamic_pointer_cast<const ReelMXF> (i);
		shared_ptr<const ReelAsset> asset = boost::dynamic_pointer_cast<const ReelAsset> (i);
		if (asset && mxf) {
			if (!mxf->key_id ()) {
				throw NotEncryptedError (asset->id ());
			}
			_keys.push_back (DecryptedKDMKey (mxf->key_type(), mxf->key_id().get(), key, cpl->id ()));
		}
	}
}

EncryptedKDM
DecryptedKDM::encrypt (shared_ptr<const Signer> signer, Certificate recipient, Formulation formulation) const
{
	list<pair<string, string> > key_ids;
	list<string> keys;
	BOOST_FOREACH (DecryptedKDMKey const & i, _keys) {
		key_ids.push_back (make_pair (i.type(), i.id ()));

		/* XXX: SMPTE only */
		uint8_t block[138];
		uint8_t* p = block;

		/* Magic value specified by SMPTE S430-1-2006 */
		uint8_t structure_id[] = { 0xf1, 0xdc, 0x12, 0x44, 0x60, 0x16, 0x9a, 0x0e, 0x85, 0xbc, 0x30, 0x06, 0x42, 0xf8, 0x66, 0xab };
		put (&p, structure_id, 16);

		base64_decode (signer->certificates().leaf().thumbprint (), p, 20);
		p += 20;

		put_uuid (&p, i.cpl_id ());
		put (&p, i.type ());
		put_uuid (&p, i.id ());
		put (&p, _not_valid_before.as_string ());
		put (&p, _not_valid_after.as_string ());
		put (&p, i.key().value(), ASDCP::KeyLen);

		/* Encrypt using the projector's public key */
		RSA* rsa = recipient.public_key ();
		unsigned char encrypted[RSA_size(rsa)];
		int const encrypted_len = RSA_public_encrypt (p - block, block, encrypted, rsa, RSA_PKCS1_OAEP_PADDING);
		if (encrypted_len == -1) {
			throw MiscError (String::compose ("Could not encrypt KDM (%1)", ERR_error_string (ERR_get_error(), 0)));
		}

		/* Lazy overallocation */
		char out[encrypted_len * 2];
		Kumu::base64encode (encrypted, encrypted_len, out, encrypted_len * 2);
		int const N = strlen (out);
		stringstream lines;
		for (int i = 0; i < N; ++i) {
			if (i > 0 && (i % 64) == 0) {
				lines << "\n";
			}
			lines << out[i];
		}

		keys.push_back (lines.str ());
	}

	string device_list_description = recipient.common_name ();
	if (device_list_description.find (".") != string::npos) {
		device_list_description = device_list_description.substr (device_list_description.find (".") + 1);
	}

	return EncryptedKDM (
		signer,
		recipient,
		device_list_description,
		_keys.front().cpl_id (),
		_content_title_text,
		_not_valid_before,
		_not_valid_after,
		formulation,
		key_ids,
		keys
		);
}
