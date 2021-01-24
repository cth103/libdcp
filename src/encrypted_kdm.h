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


/** @file  src/encrypted_kdm.h
 *  @brief EncryptedKDM class
 */


#ifndef LIBDCP_ENCRYPTED_KDM_H
#define LIBDCP_ENCRYPTED_KDM_H


#include "local_time.h"
#include "types.h"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/date_time/local_time/local_time.hpp>


namespace cxml {
	class Node;
}


namespace dcp {


namespace data {
	class EncryptedKDMData;
}


class CertificateChain;
class Certificate;


/** @class EncryptedKDM
 *  @brief An encrypted KDM
 *
 *  This is a KDM whose keys are encrypted using the target projector's private key.
 *  An EncryptedKDM object can be initialised from a KDM XML file, or created from
 *  a DecryptedKDM (using DecryptedKDM::encrypt).
 */
class EncryptedKDM
{
public:
	explicit EncryptedKDM (std::string);
	EncryptedKDM (EncryptedKDM const & kdm);
	EncryptedKDM & operator= (EncryptedKDM const &);
	~EncryptedKDM ();

	/** Write this KDM as XML to a file.
	 *  @param file File to write to.
	 */
	void as_xml (boost::filesystem::path file) const;

	/** @return This KDM as XML */
	std::string as_xml () const;

	/** @return The base64-encoded and encrypted keys that this KDM delivers.
	 *  Note that the returned `keys' contain more than just the asset decryption
	 *  keys (also key id, CPL id etc.)
	 */
	std::vector<std::string> keys () const;

	std::string id () const;
	boost::optional<std::string> annotation_text () const;
	std::string content_title_text () const;
	std::string issue_date () const;
	std::string cpl_id () const;
	LocalTime not_valid_before () const;
	LocalTime not_valid_after () const;
	std::string recipient_x509_subject_name () const;
	CertificateChain signer_certificate_chain () const;

private:

	friend class DecryptedKDM;

	/** Construct an EncryptedKDM from a set of details */
	EncryptedKDM (
		std::shared_ptr<const CertificateChain> signer,
		Certificate recipient,
		std::vector<std::string> trusted_devices,
		std::string cpl_id,
		std::string cpl_content_title_text,
		boost::optional<std::string> annotation_text,
		LocalTime not_valid_before,
		LocalTime not_valid_after,
		Formulation formulation,
		bool disable_forensic_marking_picture,
		boost::optional<int> disable_forensic_marking_audio,
		std::vector<std::pair<std::string, std::string>> key_ids,
		std::vector<std::string> keys
		);

	data::EncryptedKDMData* _data = nullptr;
};


extern bool operator== (EncryptedKDM const & a, EncryptedKDM const & b);

}


#endif
