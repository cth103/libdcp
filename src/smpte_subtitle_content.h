/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

#include "subtitle_content.h"
#include <boost/filesystem.hpp>

namespace dcp {

class SMPTELoadFontNode;

class SMPTESubtitleContent : public SubtitleContent
{
public:
	/** @param file File name
	 *  @param mxf true if `file' is a MXF, or false if it is an XML file.
	 */
	SMPTESubtitleContent (boost::filesystem::path file, bool mxf = true);

	std::list<boost::shared_ptr<LoadFontNode> > load_font_nodes () const;

	static bool valid_mxf (boost::filesystem::path);
	
private:
	std::list<boost::shared_ptr<SMPTELoadFontNode> > _load_font_nodes;
};

}
