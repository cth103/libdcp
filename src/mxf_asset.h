/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_MXF_ASSET_H
#define LIBDCP_MXF_ASSET_H

#include <boost/signals2.hpp>
#include "asset.h"
#include "key.h"

namespace ASDCP {
	class AESEncContext;
	class AESDecContext;
}

namespace libdcp
{

class MXFMetadata;	

/** @brief Parent class for assets which have MXF files */	
class MXFAsset : public Asset
{
public:
	/** Construct an MXFAsset.
	 *  This class will not write anything to disk in this constructor, but subclasses may.
	 *
	 *  @param directory Directory where MXF file is.
	 *  @param file_name Name of MXF file.
	 */
	MXFAsset (boost::filesystem::path directory, std::string file_name);
	
	/** Construct an MXFAsset.
	 *  This class will not write anything to disk in this constructor, but subclasses may.
	 *
	 *  @param directory Directory where MXF file is.
	 *  @param file_name Name of MXF file.
	 *  @param progress Signal to use to inform of progress, or 0.
	 *  @param edit_rate Edit rate in frames per second (usually equal to the video frame rate).
	 *  @param intrinsic_duration Duration of the whole asset in frames.
	 */
	MXFAsset (
		boost::filesystem::path directory,
		std::string file_name,
		boost::signals2::signal<void (float)>* progress,
		int edit_rate,
		int intrinsic_duration
		);

	~MXFAsset ();

	virtual bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;

	virtual void write_to_cpl (xmlpp::Element *, bool interop) const;

	/** Fill in a ADSCP::WriteInfo struct.
	 *  @param w struct to fill in.
	 *  @param uuid uuid to use.
	 *  @param true to label as interop, false for SMPTE.
	 */
	void fill_writer_info (ASDCP::WriterInfo* w, std::string uuid, bool interop, MXFMetadata const & metadata);

	bool encrypted () const {
		return !_key_id.empty ();
	}

	void set_key_id (std::string i) {
		_key_id = i;
	}

	std::string key_id () const {
		return _key_id;
	}
	
	void set_key (Key);

	boost::optional<Key> key () const {
		return _key;
	}

	ASDCP::AESEncContext* encryption_context () const {
		return _encryption_context;
	}

	virtual std::string key_type () const = 0;
	
protected:
	virtual std::string cpl_node_name () const = 0;
	virtual std::pair<std::string, std::string> cpl_node_attribute (bool) const {
		return std::make_pair ("", "");
	}
	
	/** Signal to emit to report progress, or 0 */
	boost::signals2::signal<void (float)>* _progress;
	ASDCP::AESEncContext* _encryption_context;
	ASDCP::AESDecContext* _decryption_context;
	std::string _key_id;
	boost::optional<Key> _key;
};

}

#endif
