/*
    Copyright (C) 2013-2016 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_DECRYPTED_KDM_H
#define LIBDCP_DECRYPTED_KDM_H

/** @file  src/decrypted_kdm.h
 *  @brief DecryptedKDM class.
 */

#include "key.h"
#include "local_time.h"
#include "decrypted_kdm_key.h"
#include "types.h"
#include "certificate.h"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

namespace dcp {

class DecryptedKDMKey;
class EncryptedKDM;
class CertificateChain;
class CPL;

/** @class DecryptedKDM
 *  @brief A decrypted KDM.
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

	/** Create a DecryptedKDM by taking a CPL and setting up to encrypt each of its
	 *  assets with the same symmetric key.
	 *
	 *  @param cpl CPL that the keys are for.
	 *  @param key Key that was used to encrypt the assets.
	 *  @param not_valid_before Start time for the KDM.
	 *  @param not_valid_after End time for the KDM.
	 */
	DecryptedKDM (
		boost::shared_ptr<const CPL> cpl,
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
	 *  @param trusted_devices Extra trusted devices which should be written to the KDM (recipient will be written
	 *  as a trusted device automatically and does not need to be included in this list).
	 *  @param formulation Formulation to use for the encrypted KDM.
	 *  @return Encrypted KDM.
	 */
	EncryptedKDM encrypt (
		boost::shared_ptr<const CertificateChain> signer,
		Certificate recipient,
		std::vector<Certificate> trusted_devices,
		Formulation formulation
		) const;

	void add_key (std::string type, std::string key_id, Key key, std::string cpl_id);
	void add_key (DecryptedKDMKey key);

	/** @return This KDM's (decrypted) keys, which could be used to decrypt assets. */
	std::list<DecryptedKDMKey> keys () const {
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
	LocalTime _not_valid_before;
	LocalTime _not_valid_after;
	boost::optional<std::string> _annotation_text;
	std::string _content_title_text;
	std::string _issue_date;
	std::list<DecryptedKDMKey> _keys;
};

}

#endif
