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
#include "types.h"

namespace ASDCP {
	class WriterInfo;
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
	Asset (std::string directory, std::string file_name = "");

	/** Write details of the asset to a CPL stream.
	 *  @param s Stream.
	 */
	virtual void write_to_cpl (std::ostream& s) const = 0;

	/** Write details of the asset to a PKL stream.
	 *  @param s Stream.
	 */
	void write_to_pkl (std::ostream& s) const;

	/** Write details of the asset to a ASSETMAP stream.
	 *  @param s Stream.
	 */
	void write_to_assetmap (std::ostream& s) const;

	std::string uuid () const {
		return _uuid;
	}

	virtual bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, std::list<std::string>& notes) const = 0;

protected:
	friend class PictureAsset;
	friend class SoundAsset;
	
	std::string digest () const;
	boost::filesystem::path path () const;

	/** Directory that our MXF or XML file is in */
	std::string _directory;
	/** Name of our MXF or XML file */
	std::string _file_name;
	/** Our UUID */
	std::string _uuid;

private:	
	/** Digest of our MXF or XML file */
	mutable std::string _digest;
};

}

#endif
