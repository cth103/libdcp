/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_MXF_H
#define LIBDCP_MXF_H

#include "asset.h"
#include "key.h"
#include "metadata.h"

#include <boost/signals2.hpp>

namespace ASDCP {
	class AESEncContext;
	class AESDecContext;
	class WriterInfo;
}

/* Undefine some stuff that the OS X 10.5 SDK defines */
#undef Key
#undef set_key

namespace dcp
{

class MXFMetadata;	

/** @class MXF
 *  @brief Parent class for classes which represent MXF files.
 */
class MXF : public Asset
{
public:
	MXF ();
	MXF (boost::filesystem::path file);
	~MXF ();

	bool equals (
		boost::shared_ptr<const Asset> other,
		EqualityOptions opt,
		NoteHandler note
		) const;

	/** Fill in a ADSCP::WriteInfo struct.
	 *  @param w struct to fill in.
	 *  @param standard INTEROP or SMPTE.
	 */
	void fill_writer_info (ASDCP::WriterInfo* w, Standard standard);

	/** @return true if the data is encrypted */
	bool encrypted () const {
		return !_key_id.empty ();
	}

	/** Set the ID of the key that is used for encryption/decryption.
	 *  @param i key ID.
	 */
	void set_key_id (std::string i) {
		_key_id = i;
	}

	/** @return the ID of the key used for encryption/decryption, or an empty string */
	std::string key_id () const {
		return _key_id;
	}

	void set_key (Key);

	/** @return encryption/decryption key, if one has been set */
	boost::optional<Key> key () const {
		return _key;
	}

	/** @return encryption context, set up with any key that has been passed to set_key() */
	ASDCP::AESEncContext* encryption_context () const {
		return _encryption_context;
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
	
protected:
	friend class MXFWriter;

	void read_writer_info (ASDCP::WriterInfo const &);
	
	ASDCP::AESEncContext* _encryption_context;
	ASDCP::AESDecContext* _decryption_context;
	/** ID of the key used for encryption/decryption, or an empty string */
	std::string _key_id;
	/** Key used for encryption/decryption, if there is one */
	boost::optional<Key> _key;
	MXFMetadata _metadata;
};

}

#endif
