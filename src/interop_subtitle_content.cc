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

#include "interop_subtitle_content.h"
#include "interop_load_font_node.h"
#include "xml.h"
#include "raw_convert.h"
#include "font_node.h"
#include <boost/foreach.hpp>
#include <cmath>

using std::list;
using std::string;
using std::cout;
using boost::shared_ptr;
using boost::optional;
using boost::dynamic_pointer_cast;
using namespace dcp;

InteropSubtitleContent::InteropSubtitleContent (boost::filesystem::path file)
	: SubtitleContent (file)
{
	shared_ptr<cxml::Document> xml (new cxml::Document ("DCSubtitle"));
	xml->read_file (file);
	_id = xml->string_child ("SubtitleID");

	_movie_title = xml->string_child ("MovieTitle");
	_load_font_nodes = type_children<dcp::InteropLoadFontNode> (xml, "LoadFont");

	list<cxml::NodePtr> f = xml->node_children ("Font");
	list<shared_ptr<dcp::FontNode> > font_nodes;
	BOOST_FOREACH (cxml::NodePtr& i, f) {
		font_nodes.push_back (shared_ptr<FontNode> (new FontNode (i, 250)));
	}

	parse_common (xml, font_nodes);
}

InteropSubtitleContent::InteropSubtitleContent (string movie_title, string language)
	: _movie_title (movie_title)
{
	_language = language;
}

struct SubtitleSorter {
	bool operator() (SubtitleString const & a, SubtitleString const & b) {
		if (a.in() != b.in()) {
			return a.in() < b.in();
		}
		return a.v_position() < b.v_position();
	}
};

Glib::ustring
InteropSubtitleContent::xml_as_string () const
{
	xmlpp::Document doc;
	xmlpp::Element* root = doc.create_root_node ("DCSubtitle");
	root->set_attribute ("Version", "1.0");

	root->add_child("SubtitleID")->add_child_text (_id);
	root->add_child("MovieTitle")->add_child_text (_movie_title);
	root->add_child("ReelNumber")->add_child_text (raw_convert<string> (_reel_number));
	root->add_child("Language")->add_child_text (_language);

	for (list<shared_ptr<InteropLoadFontNode> >::const_iterator i = _load_font_nodes.begin(); i != _load_font_nodes.end(); ++i) {
		xmlpp::Element* load_font = root->add_child("LoadFont");
		load_font->set_attribute ("Id", (*i)->id);
		load_font->set_attribute ("URI", (*i)->uri);
	}

	list<SubtitleString> sorted = _subtitles;
	sorted.sort (SubtitleSorter ());

	/* XXX: script, underlined, weight not supported */

	optional<string> font;
	bool italic = false;
	Colour colour;
	int size = 0;
	float aspect_adjust = 1.0;
	Effect effect = NONE;
	Colour effect_colour;
	int spot_number = 1;
	Time last_in;
	Time last_out;
	Time last_fade_up_time;
	Time last_fade_down_time;

	xmlpp::Element* font_element = 0;
	xmlpp::Element* subtitle_element = 0;

	for (list<SubtitleString>::iterator i = sorted.begin(); i != sorted.end(); ++i) {

		/* We will start a new <Font>...</Font> whenever some font property changes.
		   I suppose we should really make an optimal hierarchy of <Font> tags, but
		   that seems hard.
		*/

		bool const font_changed =
			font          != i->font()          ||
			italic        != i->italic()        ||
			colour        != i->colour()        ||
			size          != i->size()          ||
			fabs (aspect_adjust - i->aspect_adjust()) > ASPECT_ADJUST_EPSILON ||
			effect        != i->effect()        ||
			effect_colour != i->effect_colour();

		if (font_changed) {
			font = i->font ();
			italic = i->italic ();
			colour = i->colour ();
			size = i->size ();
			aspect_adjust = i->aspect_adjust ();
			effect = i->effect ();
			effect_colour = i->effect_colour ();
		}

		if (!font_element || font_changed) {
			font_element = root->add_child ("Font");
			if (font) {
				font_element->set_attribute ("Id", font.get ());
			}
			font_element->set_attribute ("Italic", italic ? "yes" : "no");
			font_element->set_attribute ("Color", colour.to_argb_string());
			font_element->set_attribute ("Size", raw_convert<string> (size));
			if (fabs (aspect_adjust - 1.0) > ASPECT_ADJUST_EPSILON) {
				font_element->set_attribute ("AspectAdjust", raw_convert<string> (aspect_adjust));
			}
			font_element->set_attribute ("Effect", effect_to_string (effect));
			font_element->set_attribute ("EffectColor", effect_colour.to_argb_string());
			font_element->set_attribute ("Script", "normal");
			font_element->set_attribute ("Underlined", "no");
			font_element->set_attribute ("Weight", "normal");
		}

		if (!subtitle_element || font_changed ||
		    (last_in != i->in() ||
		     last_out != i->out() ||
		     last_fade_up_time != i->fade_up_time() ||
		     last_fade_down_time != i->fade_down_time()
			    )) {

			subtitle_element = font_element->add_child ("Subtitle");
			subtitle_element->set_attribute ("SpotNumber", raw_convert<string> (spot_number++));
			subtitle_element->set_attribute ("TimeIn", i->in().as_string());
			subtitle_element->set_attribute ("TimeOut", i->out().as_string());
			subtitle_element->set_attribute ("FadeUpTime", raw_convert<string> (i->fade_up_time().as_editable_units(250)));
			subtitle_element->set_attribute ("FadeDownTime", raw_convert<string> (i->fade_down_time().as_editable_units(250)));

			last_in = i->in ();
			last_out = i->out ();
			last_fade_up_time = i->fade_up_time ();
			last_fade_down_time = i->fade_down_time ();
		}

		xmlpp::Element* text = subtitle_element->add_child ("Text");
		if (i->h_align() != HALIGN_CENTER) {
			text->set_attribute ("HAlign", halign_to_string (i->h_align ()));
		}
		if (i->h_position() > ALIGN_EPSILON) {
			text->set_attribute ("HPosition", raw_convert<string> (i->h_position() * 100, 6));
		}
		text->set_attribute ("VAlign", valign_to_string (i->v_align()));		
		text->set_attribute ("VPosition", raw_convert<string> (i->v_position() * 100, 6));
		text->add_child_text (i->text());
	}

	return doc.write_to_string_formatted ("UTF-8");
}

void
InteropSubtitleContent::add_font (string id, string uri)
{
	_load_font_nodes.push_back (shared_ptr<InteropLoadFontNode> (new InteropLoadFontNode (id, uri)));
}

bool
InteropSubtitleContent::equals (shared_ptr<const Asset> other_asset, EqualityOptions options, NoteHandler note) const
{
	if (!SubtitleContent::equals (other_asset, options, note)) {
		return false;
	}
	
	shared_ptr<const InteropSubtitleContent> other = dynamic_pointer_cast<const InteropSubtitleContent> (other_asset);
	if (!other) {
		return false;
	}

	list<shared_ptr<InteropLoadFontNode> >::const_iterator i = _load_font_nodes.begin ();
	list<shared_ptr<InteropLoadFontNode> >::const_iterator j = other->_load_font_nodes.begin ();

	while (i != _load_font_nodes.end ()) {
		if (j == other->_load_font_nodes.end ()) {
			note (DCP_ERROR, "<LoadFont> nodes differ");
			return false;
		}

		if (**i != **j) {
			note (DCP_ERROR, "<LoadFont> nodes differ");
			return false;
		}

		++i;
		++j;
	}

	if (_movie_title != other->_movie_title) {
		note (DCP_ERROR, "Subtitle movie titles differ");
		return false;
	}

	return true;
}

list<shared_ptr<LoadFontNode> >
InteropSubtitleContent::load_font_nodes () const
{
	list<shared_ptr<LoadFontNode> > lf;
	copy (_load_font_nodes.begin(), _load_font_nodes.end(), back_inserter (lf));
	return lf;
}
