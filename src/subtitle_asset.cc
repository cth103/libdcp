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

	_fonts = sub_nodes<Font> ("Font");
}

Font::Font (xmlpp::Node const * node)
	: XMLNode (node)
{
	_subtitles = sub_nodes<Subtitle> ("Subtitle");
}

Subtitle::Subtitle (xmlpp::Node const * node)
	: XMLNode (node)
{
	_in = time_attribute ("TimeIn");
	_out = time_attribute ("TimeOut");
	_texts = sub_nodes<Text> ("Text");
}

Text::Text (xmlpp::Node const * node)
	: XMLNode (node)
{
	_text = content ();
	_v_position = float_attribute ("VPosition");
}

list<shared_ptr<Text> >
SubtitleAsset::subtitles_at (Time t) const
{
	for (list<shared_ptr<Font> >::const_iterator i = _fonts.begin(); i != _fonts.end(); ++i) {
		list<shared_ptr<Text> > s = (*i)->subtitles_at (t);
		if (!s.empty ()) {
			return s;
		}
	}

	return list<shared_ptr<Text> > ();
}

list<shared_ptr<Text> >
Font::subtitles_at (Time t) const
{
	for (list<shared_ptr<Subtitle> >::const_iterator i = _subtitles.begin(); i != _subtitles.end(); ++i) {
		if ((*i)->in() <= t && t <= (*i)->out()) {
			return (*i)->texts ();
		}
	}

	return list<shared_ptr<Text> > ();
}
