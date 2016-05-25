/*
    Copyright (C) 2016 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_ATMOS_ASSET_H
#define LIBDCP_ATMOS_ASSET_H

#include "asset.h"
#include "mxf.h"

namespace dcp {

class AtmosAsset : public Asset, public MXF
{
public:
	AtmosAsset (boost::filesystem::path file);

	std::string pkl_type (Standard) const;

	Fraction edit_rate () const {
		return _edit_rate;
	}

	int64_t intrinsic_duration () const {
		return _intrinsic_duration;
	}

	/** @return frame number of the frame to align with the FFOA of the picture track */
	int first_frame () const {
		return _first_frame;
	}

	/** @return maximum number of channels in bitstream */
	int max_channel_count () const {
		return _max_channel_count;
	}

	/** @return maximum number of objects in bitstream */
	int max_object_count () const {
		return _max_object_count;
	}

private:
	Fraction _edit_rate;
	int64_t _intrinsic_duration;
	int _first_frame;
	int _max_channel_count;
	int _max_object_count;
};

}

#endif
