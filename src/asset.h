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

/** @file  src/asset.h
 *  @brief Parent class for assets of DCPs.
 */

#ifndef LIBDCP_ASSET_H
#define LIBDCP_ASSET_H

#include <string>
#include <list>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <libxml++/libxml++.h>
#include "types.h"

namespace ASDCP {
	struct WriterInfo;
}

namespace xmlpp {
	class Element;
}

namespace libdcp
{

/** @brief Parent class for assets of DCPs
 *
 *  These are collections of pictures or sound.
 */
class Asset
{
public:
	/** Construct an Asset.
	 *  @param directory Directory where our XML or MXF file is.
	 *  @param file_name Name of our file within directory, or empty to make one up based on UUID.
	 */
	Asset (boost::filesystem::path directory, boost::filesystem::path file_name = "");

	virtual ~Asset() {}

	/** Write details of the asset to a CPL AssetList node.
	 *  @param p Parent element.
	 */
	virtual void write_to_cpl (xmlpp::Element* p) const = 0;

	/** Write details of the asset to a PKL AssetList node.
	 *  @param p Parent node.
	 */
	void write_to_pkl (xmlpp::Node *, bool interop) const;

	/** Write details of the asset to a ASSETMAP stream.
	 *  @param s Stream.
	 */
	void write_to_assetmap (xmlpp::Node *) const;

	/** Compute the digest for this asset.  Calling this is optional: if
	 *  it is not called, the digest will be computed when required.  However,
	 *  calling this method allows the caller to see the progress of the
	 *  computation, which can be long for large assets.
	 *  @param Called with progress between 0 and 1.
	 */
	void compute_digest (boost::function<void (float)> progress);

	std::string uuid () const {
		return _uuid;
	}

	boost::filesystem::path path () const;

	void set_directory (boost::filesystem::path d) {
		_directory = d;
	}

	void set_file_name (boost::filesystem::path f) {
		_file_name = f;
	}

	int entry_point () const {
		return _entry_point;
	}

	int duration () const {
		return _duration;
	}
	
	int intrinsic_duration () const {
		return _intrinsic_duration;
	}
	
	int edit_rate () const {
		return _edit_rate;
	}

	void set_entry_point (int e) {
		_entry_point = e;
	}
	
	void set_duration (int d) {
		_duration = d;
	}

	void set_intrinsic_duration (int d) {
		_intrinsic_duration = d;
	}

	void set_edit_rate (int r) {
		_edit_rate = r;
	}

	virtual bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)>) const;

protected:

	/** @return Interop PKL asdcpKind for the &lt;Type&gt; tag e.g. Picture, Sound etc. */
	virtual std::string asdcp_kind () const = 0;
	
	std::string digest () const;

	/** Directory that our MXF or XML file is in */
	boost::filesystem::path _directory;
	/** Name of our MXF or XML file */
	boost::filesystem::path _file_name;
	/** Our UUID */
	std::string _uuid;
	/** The edit rate; this is normally equal to the number of video frames per second */
	int _edit_rate;
	/** Start point to present in frames */
	int _entry_point;
	/** Total length in frames */
	int _intrinsic_duration;
	/** Length to present in frames */
	int _duration;

private:	
	/** Digest of our MXF or XML file */
	mutable std::string _digest;
};

}

#endif
