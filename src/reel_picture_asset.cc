/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#include "content.h"
#include "reel_picture_asset.h"
#include "picture_mxf.h"
#include "compose.hpp"
#include <libcxml/cxml.h>

using std::bad_cast;
using std::string;
using std::stringstream;
using boost::shared_ptr;
using boost::lexical_cast;
using namespace dcp;

ReelPictureAsset::ReelPictureAsset ()
	: _frame_rate (Fraction (24, 1))
	, _screen_aspect_ratio (Fraction (1998, 1080))
{

}

ReelPictureAsset::ReelPictureAsset (boost::shared_ptr<PictureMXF> content, int64_t entry_point)
	: ReelAsset (content, entry_point)
	, _frame_rate (content->frame_rate ())
	, _screen_aspect_ratio (content->screen_aspect_ratio ())
{
	
}

ReelPictureAsset::ReelPictureAsset (boost::shared_ptr<const cxml::Node> node)
	: ReelAsset (node)
{
	_frame_rate = Fraction (node->string_child ("FrameRate"));
	try {
		_screen_aspect_ratio = Fraction (node->string_child ("ScreenAspectRatio"));
	} catch (XMLError& e) {
		/* Maybe it's not a fraction */
	}
	try {
		float f = node->number_child<float> ("ScreenAspectRatio");
		_screen_aspect_ratio = Fraction (f * 1000, 1000);
	} catch (bad_cast& e) {

	}
}

void
ReelPictureAsset::write_to_cpl (xmlpp::Node* node, Standard standard) const
{
	ReelAsset::write_to_cpl (node, standard);

	xmlpp::Node::NodeList c = node->get_children ();
	xmlpp::Node::NodeList::iterator i = c.begin();
	while (i != c.end() && (*i)->get_name() != cpl_node_name ()) {
		++i;
	}

	assert (i != c.end ());
	
	(*i)->add_child ("FrameRate")->add_child_text (String::compose ("%1 %2", _frame_rate.numerator, _frame_rate.denominator));
	if (standard == INTEROP) {
		stringstream s;
		s << std::fixed << std::setprecision (2) << (float (_screen_aspect_ratio.numerator) / _screen_aspect_ratio.denominator);
		(*i)->add_child ("ScreenAspectRatio")->add_child_text (s.str ());
	} else {
		(*i)->add_child ("ScreenAspectRatio")->add_child_text (
			String::compose ("%1 %2", _screen_aspect_ratio.numerator, _screen_aspect_ratio.denominator)
			);
	}
}
