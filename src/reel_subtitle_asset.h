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

#include "reel_asset.h"

namespace dcp {

class SubtitleContent;

class ReelSubtitleAsset : public ReelAsset
{
public:
	ReelSubtitleAsset (boost::shared_ptr<SubtitleContent> content, int64_t entry_point);
	ReelSubtitleAsset (boost::shared_ptr<const cxml::Node>);

	boost::shared_ptr<SubtitleContent> subtitle_content () const {
		return boost::dynamic_pointer_cast<SubtitleContent> (_content.object ());
	}

private:	
	std::string cpl_node_name () const;
};

}
