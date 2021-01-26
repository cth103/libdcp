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


/** @file  src/decrypted_kdm.h
 *  @brief DecryptedKDM class
 */


#ifndef LIBDCP_DECRYPTED_KDM_H
#define LIBDCP_DECRYPTED_KDM_H


#include "key.h"
#include "local_time.h"
#include "decrypted_kdm_key.h"
#include "types.h"
#include "certificate.h"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>


class decrypted_kdm_test;


namespace dcp {


class DecryptedKDMKey;
class EncryptedKDM;
class CertificateChain;
class CPL;
class ReelFileAsset;


/** @class DecryptedKDM
 *  @brief A decrypted KDM
 *
 *  This is a KDM that has either been decrypted by a target private key, or one which
 *  has been created (by some other means) ready for encryption later.
 *
 *  A DecryptedKDM object can be created either from an EncryptedKDM and private key file,
 *  or from the details of the assets that the KDM should protect.
 */
class DecryptedKDM
{
public:
	/** @param kdm Encrypted KDM.
	 *  @param private_key Private key as a PEM-format string.
	 */
	DecryptedKDM (EncryptedKDM const & kdm, std::string private_key);

	/** Create an empty DecryptedKDM.  After creation you must call
	 *  add_key() to add each key that you want in the KDM.
	 *
	 *  @param not_valid_before Start time for the KDM.
	 *  @param not_valid_after End time for the KDM.
	 */
	DecryptedKDM (
		LocalTime not_valid_before,
		LocalTime not_valid_after,
		std::string annotation_text,
		std::string content_title_text,
		std::string issue_date
		);

	/** Construct a DecryptedKDM containing a given set of keys.
	 *  @param keys Keys to be included in the DecryptedKDM.
	 */
	DecryptedKDM (
		std::string cpl_id,
		std::map<std::shared_ptr<const ReelFileAsset>, Key> keys,
		LocalTime not_valid_before,
		LocalTime not_valid_after,
		std::string annotation_text,
		std::string content_title_text,
		std::string issue_date
		);

	/** Create a DecryptedKDM by taking a CPL and setting up to encrypt each of its
	 *  assets with the same symmetric key.
	 *
	 *  @param cpl CPL that the keys are for.
	 *  @param key Key that was used to encrypt the assets.
	 *  @param not_valid_before Start time for the KDM.
	 *  @param not_valid_after End time for the KDM.
	 */
	DecryptedKDM (
		std::shared_ptr<const CPL> cpl,
		Key key,
		LocalTime not_valid_before,
		LocalTime not_valid_after,
		std::string annotation_text,
		std::string content_title_text,
		std::string issue_date
		);

	/** Encrypt this KDM's keys and sign the whole KDM.
	 *  @param signer Chain to sign with.
	 *  @param recipient Certificate of the projector/server which should receive this KDM's keys.
	 *  @param trusted_devices Thumbprints of extra trusted devices which should be written to the KDM (recipient will be written
	 *  as a trusted device automatically and does not need to be included in this list).
	 *  @param formulation Formulation to use for the encrypted KDM.
	 *  @param disable_forensic_marking_picture true to disable forensic marking of picture.
	 *  @param disable_forensic_marking_audio if not set, don't disable forensic marking of audio.  If set to 0,
	 *  disable all forensic marking; if set above 0, disable forensic marking above that channel.
	 *  @return Encrypted KDM.
	 */
	EncryptedKDM encrypt (
		std::shared_ptr<const CertificateChain> signer,
		Certificate recipient,
		std::vector<std::string> trusted_devices,
		Formulation formulation,
		bool disable_forensic_marking_picture,
		boost::optional<int> disable_forensic_marking_audio
		) const;

	/** @param type (MDIK, MDAK etc.)
	 *  @param key_id Key ID
	 *  @param key The actual symmetric key
	 *  @param cpl_id ID of CPL that the key is for
	 */
	void add_key (boost::optional<std::string> type, std::string key_id, Key key, std::string cpl_id, Standard standard);

	void add_key (DecryptedKDMKey key);

	/** @return This KDM's (decrypted) keys, which could be used to decrypt assets. */
	std::vector<DecryptedKDMKey> keys () const {
		return _keys;
	}

	boost::optional<std::string> annotation_text () const {
		return _annotation_text;
	}

	std::string content_title_text () const {
		return _content_title_text;
	}

	std::string issue_date () const {
		return _issue_date;
	}

private:

	friend class ::decrypted_kdm_test;

	static void put_uuid (uint8_t ** d, std::string id);
	static std::string get_uuid (unsigned char ** p);

	LocalTime _not_valid_before;
	LocalTime _not_valid_after;
	boost::optional<std::string> _annotation_text;
	std::string _content_title_text;
	std::string _issue_date;
	std::vector<DecryptedKDMKey> _keys;
};


}


#endif
