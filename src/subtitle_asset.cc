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

#include "subtitle_asset.h"

using namespace std;
using namespace boost;
using namespace libdcp;

SubtitleAsset::SubtitleAsset (string directory, string xml)
	: Asset (directory, xml)
	, XMLFile (path().string(), "DCSubtitle")
{
	_subtitle_id = string_node ("SubtitleID");
	_movie_title = string_node ("MovieTitle");
	_reel_number = int64_node ("ReelNumber");
	_language = string_node ("Language");

	ignore_node ("LoadFont");

	list<shared_ptr<FontNode> > font_nodes = sub_nodes<FontNode> ("Font");
	list<shared_ptr<LoadFontNode> > load_font_nodes = sub_nodes<LoadFontNode> ("LoadFont");

	/* Now make Subtitle objects to represent the raw XML nodes
	   in a sane way.
	*/

	for (list<shared_ptr<FontNode> >::iterator i = font_nodes.begin(); i != font_nodes.end(); ++i) {
		for (list<shared_ptr<SubtitleNode> >::iterator j = (*i)->subtitle_nodes.begin(); j != (*i)->subtitle_nodes.end(); ++j) {
			for (list<shared_ptr<TextNode> >::iterator k = (*j)->text_nodes.begin(); k != (*j)->text_nodes.end(); ++k) {
				_subtitles.push_back (
					shared_ptr<Subtitle> (
						new Subtitle (
							font_id_to_name ((*i)->id, load_font_nodes),
							(*i)->size,
							(*j)->in,
							(*j)->out,
							(*k)->v_position,
							(*k)->text
							)
						)
					);
			}
		}
	}
}

FontNode::FontNode (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_attribute ("Id");
	size = int64_attribute ("Size");
	subtitle_nodes = sub_nodes<SubtitleNode> ("Subtitle");
}

LoadFontNode::LoadFontNode (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_attribute ("Id");
	uri = string_attribute ("URI");
}
	

SubtitleNode::SubtitleNode (xmlpp::Node const * node)
	: XMLNode (node)
{
	in = time_attribute ("TimeIn");
	out = time_attribute ("TimeOut");
	text_nodes = sub_nodes<TextNode> ("Text");
}

TextNode::TextNode (xmlpp::Node const * node)
	: XMLNode (node)
{
	text = content ();
	v_position = float_attribute ("VPosition");
}

list<shared_ptr<Subtitle> >
SubtitleAsset::subtitles_at (Time t) const
{
	list<shared_ptr<Subtitle> > s;
	for (list<shared_ptr<Subtitle> >::const_iterator i = _subtitles.begin(); i != _subtitles.end(); ++i) {
		if ((*i)->in() <= t && t <= (*i)->out ()) {
			s.push_back (*i);
		}
	}

	return s;
}

std::string
SubtitleAsset::font_id_to_name (string id, list<shared_ptr<LoadFontNode> > const & load_font_nodes) const
{
	list<shared_ptr<LoadFontNode> >::const_iterator i = load_font_nodes.begin();
	while (i != load_font_nodes.end() && (*i)->id != id) {
		++i;
	}

	if (i == load_font_nodes.end ()) {
		return "";
	}

	if ((*i)->uri == "arial.ttf") {
		return "Arial";
	}

	return "";
}

Subtitle::Subtitle (
	std::string font,
	int size,
	Time in,
	Time out,
	float v_position,
	std::string text
	)
	: _font (font)
	, _size (size)
	, _in (in)
	, _out (out)
	, _v_position (v_position)
	, _text (text)
{

}

int
Subtitle::size_in_pixels (int screen_height) const
{
	/* Size in the subtitle file is given in points as if the screen
	   height is 11 inches, so a 72pt font would be 1/11th of the screen
	   height.
	*/
	
	return _size * screen_height / (11 * 72);
}
