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
#include <sigc++/sigc++.h>
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
	 *  @param directory Directory where MXF file is.
	 *  @param mxf_name Name of MXF file.
	 *  @param progress Signal to inform of progress.
	 *  @param fps Frames per second.
	 *  @param length Length in frames.
	 */
	Asset (std::string directory, std::string mxf_path, sigc::signal1<void, float>* progress, int fps, int length);

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

	virtual std::list<std::string> equals (boost::shared_ptr<const Asset> other, EqualityOptions opt) const;

protected:
	friend class PictureAsset;
	friend class SoundAsset;
	
	/** Fill in a ADSCP::WriteInfo struct.
	 *  @param w struct to fill in.
	 */
	void fill_writer_info (ASDCP::WriterInfo* w) const;

	boost::filesystem::path mxf_path () const;
	std::string digest () const;

	/** Directory that our MXF file is in */
	std::string _directory;
	/** Name of our MXF file */
	std::string _mxf_name;
	/** Signal to emit to report progress */
	sigc::signal1<void, float>* _progress;
	/** Frames per second */
	int _fps;
	/** Length in frames */
	int _length;
	/** Our UUID */
	std::string _uuid;

private:	
	/** Digest of our MXF */
	mutable std::string _digest;
};

}

#endif
