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

#ifndef LIBDCP_MONO_PICTURE_MXF_H
#define LIBDCP_MONO_PICTURE_MXF_H

#include "picture_mxf.h"

namespace dcp {

class MonoPictureMXFWriter;	

/** @class MonoPictureMXF
 *  @brief A 2D (monoscopic) picture MXF.
 */
class MonoPictureMXF : public PictureMXF
{
public:
	/** Create a MonoPictureMXF by reading a file.
	 *  @param file MXF file to read.
	 */
	MonoPictureMXF (boost::filesystem::path file);

	/** Create a MonoPictureMXF with a given edit rate.
	 *  @param edit_rate Edit rate (i.e. frame rate) in frames per second.
	 */
	MonoPictureMXF (Fraction edit_rate);

	/** Start a progressive write to a MonoPictureMXF */
	boost::shared_ptr<PictureMXFWriter> start_write (boost::filesystem::path, Standard standard, bool);

	bool equals (
		boost::shared_ptr<const Content> other,
		EqualityOptions opt,
		boost::function<void (NoteType, std::string)> note
		) const;
	
	boost::shared_ptr<const MonoPictureFrame> get_frame (int n) const;

private:
	std::string cpl_node_name () const;
};

}	

#endif
