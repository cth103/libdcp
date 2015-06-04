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

#ifndef LIBDCP_PICTURE_MXF_H
#define LIBDCP_PICTURE_MXF_H

/** @file  src/picture_mxf.h
 *  @brief PictureMXF class.
 */

#include "mxf.h"
#include "util.h"
#include "metadata.h"
#include <openjpeg.h>

namespace ASDCP {
	namespace JP2K {
		struct PictureDescriptor;
	}
}

namespace dcp
{

class MonoPictureFrame;	
class StereoPictureFrame;
class PictureMXFWriter;

/** @class PictureMXF
 *  @brief An asset made up of JPEG2000 data.
 */
class PictureMXF : public MXF
{
public:
	PictureMXF (boost::filesystem::path file);
	PictureMXF (Fraction edit_rate);

	virtual boost::shared_ptr<PictureMXFWriter> start_write (
		boost::filesystem::path file,
		Standard standard,
		bool overwrite
		) = 0;

	Size size () const {
		return _size;
	}

	void set_size (Size s) {
		_size = s;
	}

	Fraction frame_rate () const {
		return _frame_rate;
	}

	void set_frame_rate (Fraction r) {
		_frame_rate = r;
	}

	Fraction screen_aspect_ratio () const {
		return _screen_aspect_ratio;
	}

	void set_screen_aspect_ratio (Fraction r) {
		_screen_aspect_ratio = r;
	}

	Fraction edit_rate () const {
		return _edit_rate;
	}

	int64_t intrinsic_duration () const {
		return _intrinsic_duration;
	}

protected:
	friend class MonoPictureMXFWriter;
	friend class StereoPictureMXFWriter;

	bool frame_buffer_equals (
		int frame, EqualityOptions opt, NoteHandler note,
		uint8_t const * data_A, unsigned int size_A, uint8_t const * data_B, unsigned int size_B
		) const;

	bool descriptor_equals (
		ASDCP::JP2K::PictureDescriptor const & a,
		ASDCP::JP2K::PictureDescriptor const & b,
		NoteHandler note
		) const;

	void read_picture_descriptor (ASDCP::JP2K::PictureDescriptor const &);

	Fraction _edit_rate;
	/** The total length of this content in video frames.  The amount of
	 *  content presented may be less than this.
	 */
	int64_t _intrinsic_duration;
	/** picture size in pixels */
	Size _size;
	Fraction _frame_rate;
	Fraction _screen_aspect_ratio;

private:
	std::string pkl_type (Standard standard) const;
};
	

}

#endif
