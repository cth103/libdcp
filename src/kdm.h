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

/** @file  src/kdm.h
 *  @brief Handling of Key Delivery Messages (KDMs).
 */

#ifndef LIBDCP_KDM_H
#define LIBDCP_KDM_H

#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "key.h"
#include "metadata.h"

class kdm_key_test;

namespace libdcp {

namespace xml {
	class DCinemaSecurityMessage;
};

class Signer;
class Certificate;
class CPL;

/** @class KDMKey
 *  @brief A single key (and associated metadata) for encrypting or decrypting an MXF.
 *
 *  One or more of these are delivered (themselves encrypted) in a KDM.  The following
 *  data is collected into a block:
 *
 *  A structure ID (a magic value specified by the standard)
 *  The thumbprint of the KDM signer's certificate.
 *  The CPL ID.
 *  The key ID.
 *  Validity start and end times.
 *  The key itself
 *
 *  This data block is then encrypted using the projector's public key, so that
 *  only the target projector can decrypt block.
 */
class KDMKey
{
public:
	/** Create a KDMKey from the raw block that is encrypted in the KDM's CipherData.
	 *  @param raw Pointer to data block (134 bytes for interop, 138 bytes for SMPTE).
	 *  @param len Length of the data block in bytes.
	 */
	KDMKey (uint8_t const * raw, int len);

	/** Create a KDMKey from its constituent parts.
	 *  @param signer Signer for the KDM.
	 *  @param cpl_id ID of the CPL that the KDM is for.
	 *  @param key_type Type of data that this key is for (MDIK for image, MDAK for audio, ...)
	 *  @param key_id ID of this key.
	 *  @param from Valid-from time.
	 *  @param until Valid-until time.
	 *  @param key The key itself.
	 */
	KDMKey (
		boost::shared_ptr<const Signer> signer,
		std::string cpl_id,
		std::string key_type,
		std::string key_id,
		boost::posix_time::ptime from,
		boost::posix_time::ptime until,
		Key key
		);
	
	KDMKey (KDMKey const &);

	KDMKey& operator= (KDMKey const &);

	/** @return ID of the CPL that the KDM is for */
	std::string cpl_id () const {
		return _cpl_id;
	}
	
	/** @return ID of the key */
	std::string key_id () const {
		return _key_id;
	}

	/** @return start of the validity period as a string */
	std::string not_valid_before () const {
		return _not_valid_before;
	}

	/** @return end of the validity period as a string */
	std::string not_valid_after () const {
		return _not_valid_after;
	}

	/** @return the key itself */
	Key key () const {
		return _key;
	}

	/** @param cert Cerfificate.
	 *  @return The data block encrypted with a certificate's public key and converted to base 64.
	 */
	std::string encrypted_base64 (boost::shared_ptr<const Certificate> cert) const;
	
private:
	friend class ::kdm_key_test;
	
	void get (uint8_t *, uint8_t const **, int) const;
	std::string get (uint8_t const **, int) const;
	std::string get_uuid (uint8_t const **) const;
	void put (uint8_t **, uint8_t const *, int) const;
	void put (uint8_t **, std::string) const;
	void put_uuid (uint8_t **, std::string) const;

	friend bool operator== (KDMKey const &, KDMKey const &);
	
	uint8_t _signer_thumbprint[20];
	std::string _cpl_id;
	std::string _key_type;
	std::string _key_id;
	std::string _not_valid_before;
	std::string _not_valid_after;
	Key _key;
};

/** @class KDM
 *  @brief A class representing a Key Delivery Message (KDM).
 *
 *  A KDM wraps one or more content keys (which we wrap into KDMKey objects) and various
 *  other metadata.  This class can read and decrypt existing KDMs (provided you have
 *  the private key that the KDM was targeted at).  It can also create new KDMs for
 *  a given CPL.
 */
class KDM
{
public:
	/** Load and decrypt a KDM.  After this constructor the KDMKeys can be read
	 *  and used to decrypt MXFs.
	 *
	 *  @param kdm KDM file name.
	 *  @param private_key Private key file name.
	 */
	KDM (boost::filesystem::path kdm, boost::filesystem::path private_key);

	enum Formulation
	{
		MODIFIED_TRANSITIONAL_1,
		DCI_ANY,
		DCI_SPECIFIC
	};

	
	/** Create a new KDM.
	 *  @param cpl CPL file that the KDM is for.
	 *  @param signer Certificate chain to sign the KDM with.
	 *  @param recipient_cert Certificate of the projector that this KDM is targeted at.
	 *  @param key Key used to encrypt all MXF data.
	 *  @param not_valid_before Start of validity period.
	 *  @param not_valid_after End of validity period.
	 *  @param annotation_text Text for the <AnnotationText> node.
	 *  @param issue_date Text for the <IssueDate> node.
	 */
	KDM (
		boost::filesystem::path cpl,
		boost::shared_ptr<const Signer> signer,
		boost::shared_ptr<const Certificate> recipient_cert,
		Key key,
		boost::posix_time::ptime not_valid_before, boost::posix_time::ptime not_valid_after,
		std::string annotation_text, std::string issue_date,
		Formulation formulation
		);

	KDM (KDM const &);
	KDM & operator= (KDM const &);

	/** @return The unencrypted content keys from this KDM */
	std::list<KDMKey> keys () const {
		return _keys;
	}

	/** Write this KDM to a file.
	 *  @param file File to write to.
	 */
	void as_xml (boost::filesystem::path file) const;

	/** Obtain this KDM as an XML string.
	 *  @return XML string.
	 */
	std::string as_xml () const;

private:
	/** Unencrypted MXF content keys */
	std::list<KDMKey> _keys;

	/** The KDM's contents, mapped 1:1-ish to the XML */
	boost::shared_ptr<xml::DCinemaSecurityMessage> _xml_kdm;
};


}

#endif
