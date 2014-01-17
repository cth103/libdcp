/*
    Copyright (C) 2012-2013 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_STEREO_PICTURE_MXF_H
#define LIBDCP_STEREO_PICTURE_MXF_H

#include "picture_mxf.h"

namespace dcp {
	
/** A 3D (stereoscopic) picture asset */	
class StereoPictureMXF : public PictureMXF
{
public:
	StereoPictureMXF (boost::filesystem::path directory, boost::filesystem::path mxf_name);

	void read ();
	
	/** Start a progressive write to a StereoPictureMXF */
	boost::shared_ptr<PictureMXFWriter> start_write (bool);

	boost::shared_ptr<const StereoPictureFrame> get_frame (int n) const;
	bool equals (boost::shared_ptr<const ContentAsset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;

private:
	std::string cpl_node_name () const;
	std::pair<std::string, std::string> cpl_node_attribute () const;
	int edit_rate_factor () const;
};

}

#endif
