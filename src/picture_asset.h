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

#ifndef LIBDCP_PICTURE_ASSET_H
#define LIBDCP_PICTURE_ASSET_H

/** @file  src/picture_asset.h
 *  @brief An asset made up of JPEG2000 data
 */

#include <openjpeg.h>
#include "mxf_asset.h"
#include "util.h"
#include "metadata.h"

namespace ASDCP {
	namespace JP2K {
		struct PictureDescriptor;
	}
}

namespace libdcp
{

class MonoPictureFrame;	
class StereoPictureFrame;
class PictureAssetWriter;

/** @brief An asset made up of JPEG2000 data */
class PictureAsset : public MXFAsset
{
public:
	/** Construct a PictureAsset.
	 *  
	 *  @param directory Directory where MXF file is.
	 *  @param mxf_name Name of MXF file.
	 */
	PictureAsset (boost::filesystem::path directory, boost::filesystem::path mxf_name);

	/** Start a progressive write to this asset.
	 *  The following parameters must be set up (if required) before calling this:
	 *      Interop mode (set_interop)
	 *      Edit rate    (set_edit_rate)
	 *      MXF Metadata (set_metadata)
	 *      
	 *  @param overwrite true to overwrite an existing MXF file; in this mode, writing can be resumed to a partially-written MXF; false if the
	 *  MXF file does not exist.
	 */
	virtual boost::shared_ptr<PictureAssetWriter> start_write (bool overwrite) = 0;

	virtual void read () = 0;
	virtual void create (std::vector<boost::filesystem::path> const &) {}
	virtual void create (boost::function<boost::filesystem::path (int)>) {}
	
	Size size () const {
		return _size;
	}

	void set_size (Size s) {
		_size = s;
	}

	void write_to_cpl (xmlpp::Element *) const;

protected:

	std::string asdcp_kind () const {
		return "Picture";
	}

	bool frame_buffer_equals (
		int frame, EqualityOptions opt, boost::function<void (NoteType, std::string)> note,
		uint8_t const * data_A, unsigned int size_A, uint8_t const * data_B, unsigned int size_B
		) const;

	bool descriptor_equals (
		ASDCP::JP2K::PictureDescriptor const & a, ASDCP::JP2K::PictureDescriptor const & b, boost::function<void (NoteType, std::string)>
		) const;

	/** picture size in pixels */
	Size _size;

private:
	std::string key_type () const;
	virtual int edit_rate_factor () const = 0;
};
	

}

#endif
