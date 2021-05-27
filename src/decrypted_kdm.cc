/*
    Copyright (C) 2013-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/decrypted_kdm.cc
 *  @brief DecryptedKDM class
 */


#include "certificate_chain.h"
#include "compose.hpp"
#include "cpl.h"
#include "dcp_assert.h"
#include "decrypted_kdm.h"
#include "decrypted_kdm_key.h"
#include "encrypted_kdm.h"
#include "exceptions.h"
#include "reel_asset.h"
#include "reel_file_asset.h"
#include "util.h"
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_util.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>


using std::list;
using std::vector;
using std::string;
using std::setw;
using std::setfill;
using std::hex;
using std::pair;
using std::map;
using std::shared_ptr;
using boost::optional;
using namespace dcp;


/* Magic value specified by SMPTE S430-1-2006 */
static uint8_t smpte_structure_id[] = { 0xf1, 0xdc, 0x12, 0x44, 0x60, 0x16, 0x9a, 0x0e, 0x85, 0xbc, 0x30, 0x06, 0x42, 0xf8, 0x66, 0xab };


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


void
DecryptedKDM::put_uuid (uint8_t ** d, string id)
{
	/* 32 hex digits plus some hyphens */
	DCP_ASSERT (id.length() == 36);
#ifdef LIBDCP_WINDOWS
	__mingw_sscanf (
#else
	sscanf (
#endif
		id.c_str(),
		"%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
		*d + 0, *d + 1, *d + 2, *d + 3, *d + 4, *d + 5, *d + 6, *d + 7,
		*d + 8,	*d + 9, *d + 10, *d + 11, *d + 12, *d + 13, *d + 14, *d + 15
		);

	*d += 16;
}


string
DecryptedKDM::get_uuid (unsigned char ** p)
{
	char buffer[37];
#ifdef LIBDCP_WINDOWS
	__mingw_snprintf (
#else
	snprintf (
#endif
		buffer, sizeof(buffer), "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
		(*p)[0], (*p)[1], (*p)[2], (*p)[3], (*p)[4], (*p)[5], (*p)[6], (*p)[7],
		(*p)[8], (*p)[9], (*p)[10], (*p)[11], (*p)[12], (*p)[13], (*p)[14], (*p)[15]
		);

	*p += 16;
	return buffer;
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

	auto bio = BIO_new_mem_buf (const_cast<char *>(private_key.c_str()), -1);
	if (!bio) {
		throw MiscError ("could not create memory BIO");
	}

	auto rsa = PEM_read_bio_RSAPrivateKey (bio, 0, 0, 0);
	if (!rsa) {
		throw FileError ("could not read RSA private key file", private_key, errno);
	}

	/* Use the private key to decrypt the keys */

	for (auto const& i: kdm.keys()) {
		/* Decode the base-64-encoded cipher value from the KDM */
		unsigned char cipher_value[256];
		int const cipher_value_len = base64_decode (i, cipher_value, sizeof (cipher_value));

		/* Decrypt it */
		auto decrypted = new unsigned char[RSA_size(rsa)];
		int const decrypted_len = RSA_private_decrypt (cipher_value_len, cipher_value, decrypted, rsa, RSA_PKCS1_OAEP_PADDING);
		if (decrypted_len == -1) {
			delete[] decrypted;
#if OPENSSL_VERSION_NUMBER > 0x10100000L
			throw KDMDecryptionError (ERR_error_string (ERR_get_error(), 0), cipher_value_len, RSA_bits(rsa));
#else
			throw KDMDecryptionError (ERR_error_string (ERR_get_error(), 0), cipher_value_len, rsa->n->dmax);
#endif
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
			add_key (optional<string>(), key_id, Key(p), cpl_id, Standard::INTEROP);
			break;
		}
		case 138:
		{
			/* SMPTE */
			/* 0 is structure id (fixed sequence specified by standard) [16 bytes] */
			DCP_ASSERT (memcmp (p, smpte_structure_id, 16) == 0);
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
			add_key (key_type, key_id, Key(p), cpl_id, Standard::SMPTE);
			break;
		}
		default:
			DCP_ASSERT (false);
		}

		delete[] decrypted;
	}

	RSA_free (rsa);
	BIO_free (bio);

	_annotation_text = kdm.annotation_text ();
	_content_title_text = kdm.content_title_text ();
	_issue_date = kdm.issue_date ();
}


DecryptedKDM::DecryptedKDM (
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

}


DecryptedKDM::DecryptedKDM (
	string cpl_id,
	map<shared_ptr<const ReelFileAsset>, Key> keys,
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
	for (auto const& i: keys) {
		add_key (i.first->key_type(), i.first->key_id().get(), i.second, cpl_id, Standard::SMPTE);
	}
}


DecryptedKDM::DecryptedKDM (
	shared_ptr<const CPL> cpl,
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
	bool did_one = false;
	for (auto i: cpl->reel_file_assets()) {
		if (i->encryptable()) {
			add_key (i->key_type().get(), i->key_id().get(), key, cpl->id(), Standard::SMPTE);
			did_one = true;
		}
	}

	if (!did_one) {
		throw NotEncryptedError (cpl->id ());
	}
}


void
DecryptedKDM::add_key (optional<string> type, string key_id, Key key, string cpl_id, Standard standard)
{
	_keys.push_back (DecryptedKDMKey (type, key_id, key, cpl_id, standard));
}


void
DecryptedKDM::add_key (DecryptedKDMKey key)
{
	_keys.push_back (key);
}


EncryptedKDM
DecryptedKDM::encrypt (
	shared_ptr<const CertificateChain> signer,
	Certificate recipient,
	vector<string> trusted_devices,
	Formulation formulation,
	bool disable_forensic_marking_picture,
	optional<int> disable_forensic_marking_audio
	) const
{
	DCP_ASSERT (!_keys.empty ());

	for (auto i: signer->leaf_to_root()) {
		if (day_greater_than_or_equal(dcp::LocalTime(i.not_before()), _not_valid_before)) {
			throw BadKDMDateError (true);
		} else if (day_less_than_or_equal(dcp::LocalTime(i.not_after()), _not_valid_after)) {
			throw BadKDMDateError (false);
		}
	}

	vector<pair<string, string>> key_ids;
	vector<string> keys;
	for (auto const& i: _keys) {
		/* We're making SMPTE keys so we must have a type for each one */
		DCP_ASSERT (i.type());
		key_ids.push_back (make_pair (i.type().get(), i.id ()));

		/* XXX: SMPTE only */
		uint8_t block[138];
		uint8_t* p = block;

		put (&p, smpte_structure_id, 16);

		base64_decode (signer->leaf().thumbprint (), p, 20);
		p += 20;

		put_uuid (&p, i.cpl_id ());
		put (&p, i.type().get());
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
		string lines;
		for (int i = 0; i < N; ++i) {
			if (i > 0 && (i % 64) == 0) {
				lines += "\n";
			}
			lines += out[i];
		}

		keys.push_back (lines);
	}

	string device_list_description = recipient.subject_common_name ();
	if (device_list_description.find (".") != string::npos) {
		device_list_description = device_list_description.substr (device_list_description.find (".") + 1);
	}

	return EncryptedKDM (
		signer,
		recipient,
		trusted_devices,
		_keys.front().cpl_id (),
		_content_title_text,
		_annotation_text,
		_not_valid_before,
		_not_valid_after,
		formulation,
		disable_forensic_marking_picture,
		disable_forensic_marking_audio,
		key_ids,
		keys
		);
}
