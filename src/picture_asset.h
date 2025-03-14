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


#ifndef LIBDCP_PICTURE_ASSET_H
#define LIBDCP_PICTURE_ASSET_H


#include "asset.h"
#include "mxf.h"
#include "types.h"


namespace dcp {


class PictureAsset : public Asset, public MXF
{
public:
	explicit PictureAsset(boost::filesystem::path file);
	PictureAsset(Fraction edit_rate, Standard standard);

	Fraction edit_rate () const {
		return _edit_rate;
	}

	int64_t intrinsic_duration () const {
		return _intrinsic_duration;
	}

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

protected:
	Fraction _edit_rate;
	/** The total length of this content in video frames.  The amount of
	 *  content presented may be less than this.
	 */
	int64_t _intrinsic_duration = 0;
	/** picture size in pixels */
	Size _size;
	Fraction _frame_rate;
	Fraction _screen_aspect_ratio;

};


}


#endif

