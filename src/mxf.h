/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/mxf.h
 *  @brief MXF class
 */


#ifndef LIBDCP_MXF_H
#define LIBDCP_MXF_H


#include "asset.h"
#include "key.h"
#include "metadata.h"
#include "dcp_assert.h"
#include <boost/signals2.hpp>


namespace ASDCP {
	class AESDecContext;
	struct WriterInfo;
}


/* Undefine some stuff that the OS X 10.5 SDK defines */
#undef Key
#undef set_key


namespace dcp
{


class MXFMetadata;
class PictureAssetWriter;


/** @class MXF
 *  @brief Parent for classes which represent MXF files
 */
class MXF
{
public:
	MXF (Standard standard);
	virtual ~MXF () {}

	/** @return true if the data is encrypted */
	bool encrypted () const {
		return static_cast<bool>(_key_id);
	}

	/** Set the ID of the key that is used for encryption/decryption.
	 *  @param i key ID.
	 */
	void set_key_id (std::string i) {
		_key_id = i;
	}

	/** @return the ID of the key used for encryption/decryption, if there is one */
	boost::optional<std::string> key_id () const {
		return _key_id;
	}

	/** Set the (private) key that will be used to encrypt or decrypt this MXF's content
	 *  This is the top-secret key that is distributed (itself encrypted) to cinemas
	 *  via Key Delivery Messages (KDMs)
	 *  @param key Key to use
	 */
	virtual void set_key (Key);

	/** @return encryption/decryption key, if one has been set */
	boost::optional<Key> key () const {
		return _key;
	}

	/** Set the context ID to be used when encrypting.
	 *  @param id New ID.
	 */
	void set_context_id (std::string id) {
		_context_id = id;
	}

	/** @return context ID used when encrypting; this starts off as a random value */
	std::string context_id () const {
		return _context_id;
	}

	/** Set the metadata that is written to the MXF file.
	 *  @param m Metadata.
	 */
	void set_metadata (MXFMetadata m) {
		_metadata = m;
	}

	/** @return metadata from the MXF file */
	MXFMetadata metadata () const {
		return _metadata;
	}

	Standard standard () const {
		DCP_ASSERT (_standard);
		return *_standard;
	}

protected:
	template <class P, class Q>
	friend void start (PictureAssetWriter* writer, std::shared_ptr<P> state, Q* mxf, uint8_t const * data, int size);

	MXF ();

	/** Read an ASDCP::WriterInfo struct, extracting things for our
	 *  member variables.
	 *  @return AssetUUID of the MXF
	 */
	std::string read_writer_info (ASDCP::WriterInfo const &);

	/** Fill in a ASDCP::WriteInfo struct.
	 *  @param w struct to fill in
	 */
	void fill_writer_info (ASDCP::WriterInfo* w, std::string id) const;

	/** ID of the key used for encryption/decryption, if there is one */
	boost::optional<std::string> _key_id;
	/** Key used for encryption/decryption, if there is one */
	boost::optional<Key> _key;
	std::string _context_id;
	MXFMetadata _metadata;
	boost::optional<Standard> _standard;
};


}


#endif
