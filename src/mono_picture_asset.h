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


/** @file  src/mono_picture_asset.cc
 *  @brief MonoPictureAsset class
 */


#ifndef LIBDCP_MONO_PICTURE_ASSET_H
#define LIBDCP_MONO_PICTURE_ASSET_H


#include "picture_asset.h"
#include "mono_picture_asset_reader.h"


namespace dcp {


class MonoPictureAssetWriter;


/** @class MonoPictureAsset
 *  @brief A 2D (monoscopic) picture asset
 */
class MonoPictureAsset : public PictureAsset
{
public:
	/** Create a MonoPictureAsset by reading a file.
	 *  @param file Asset file to read.
	 */
	explicit MonoPictureAsset (boost::filesystem::path file);

	/** Create a MonoPictureAsset with a given edit rate.
	 *  @param edit_rate Edit rate (i.e. frame rate) in frames per second.
	 *  @param standard DCP standard (INTEROP or SMPTE).
	 */
	explicit MonoPictureAsset (Fraction edit_rate, Standard standard);

	/** Start a progressive write to a MonoPictureAsset.
	 *  @path file File to write to.
	 *  @path overwrite true to overwrite an existing file; for use when continuing a write which
	 *  previously failed.  If in doubt, use false here.
	 */
	std::shared_ptr<PictureAssetWriter> start_write (boost::filesystem::path file, bool overwrite) override;
	std::shared_ptr<MonoPictureAssetReader> start_read () const;

	bool equals (
		std::shared_ptr<const Asset> other,
		EqualityOptions opt,
		NoteHandler note
		) const override;

private:
	std::string cpl_node_name () const;
};


}


#endif
