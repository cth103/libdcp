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


/** @file  src/j2k_picture_asset.h
 *  @brief J2KPictureAsset class
 */


#ifndef LIBDCP_J2K_PICTURE_ASSET_H
#define LIBDCP_J2K_PICTURE_ASSET_H


#include "behaviour.h"
#include "mxf.h"
#include "metadata.h"
#include "picture_asset.h"
#include "util.h"


namespace ASDCP {
	namespace JP2K {
		struct PictureDescriptor;
	}
}


namespace dcp {


class MonoJ2KPictureFrame;
class StereoJ2KPictureFrame;
class J2KPictureAssetWriter;


/** @class J2KPictureAsset
 *  @brief An asset made up of JPEG2000 data
 */
class J2KPictureAsset : public PictureAsset
{
public:
	/** Load a J2KPictureAsset from a file */
	explicit J2KPictureAsset (boost::filesystem::path file);

	/** Create a new J2KPictureAsset with a given edit rate and standard */
	J2KPictureAsset (Fraction edit_rate, Standard standard);

	virtual std::shared_ptr<J2KPictureAssetWriter> start_write (
		boost::filesystem::path file,
		Behaviour behaviour
		) = 0;

	static std::string static_pkl_type (Standard standard);

protected:
	friend class MonoJ2KPictureAssetWriter;
	friend class StereoJ2KPictureAssetWriter;

	bool frame_buffer_equals (
		int frame, EqualityOptions const& opt, NoteHandler note,
		uint8_t const * data_A, unsigned int size_A, uint8_t const * data_B, unsigned int size_B
		) const;

	bool descriptor_equals (
		ASDCP::JP2K::PictureDescriptor const & a,
		ASDCP::JP2K::PictureDescriptor const & b,
		NoteHandler note
		) const;

	void read_picture_descriptor (ASDCP::JP2K::PictureDescriptor const &);

private:
	std::string pkl_type (Standard standard) const override;
};


}


#endif
