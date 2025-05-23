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


/** @file  src/mono_j2k_picture_asset.h
 *  @brief MonoJ2KPictureAsset class
 */


#ifndef LIBDCP_MONO_J2K_PICTURE_ASSET_H
#define LIBDCP_MONO_J2K_PICTURE_ASSET_H


#include "j2k_picture_asset.h"
#include "mono_j2k_picture_asset_reader.h"


namespace dcp {


class MonoJ2KPictureAssetWriter;


/** @class MonoJ2KPictureAsset
 *  @brief A 2D (monoscopic) picture asset
 */
class MonoJ2KPictureAsset : public J2KPictureAsset
{
public:
	/** Create a MonoJ2KPictureAsset by reading a file.
	 *  @param file Asset file to read.
	 */
	explicit MonoJ2KPictureAsset (boost::filesystem::path file);

	/** Create a MonoJ2KPictureAsset with a given edit rate.
	 *  @param edit_rate Edit rate (i.e. frame rate) in frames per second.
	 *  @param standard DCP standard (INTEROP or SMPTE).
	 */
	MonoJ2KPictureAsset(Fraction edit_rate, Standard standard);

	/** Start a progressive write to a MonoJ2KPictureAsset.
	 *  @path file File to write to.
	 *  @path behaviour OVERWRITE_EXISTING to overwrite and potentially add to an existing file
	 *  (after a write previously failed), MAKE_NEW to create a new file.
	 *  If in doubt, use MAKE_NEW here.
	 */
	std::shared_ptr<J2KPictureAssetWriter> start_write(boost::filesystem::path file, Behaviour behaviour) override;
	std::shared_ptr<MonoJ2KPictureAssetReader> start_read () const;

	bool equals (
		std::shared_ptr<const Asset> other,
		EqualityOptions const& opt,
		NoteHandler note
		) const override;

private:
	std::string cpl_node_name () const;
};


}


#endif
