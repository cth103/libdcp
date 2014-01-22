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

#include <fstream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <libxml++/nodes/element.h>
#include "subtitle_asset.h"
#include "parse/subtitle.h"
#include "util.h"
#include "xml.h"

using std::string;
using std::list;
using std::ostream;
using std::ofstream;
using std::stringstream;
using boost::shared_ptr;
using boost::lexical_cast;
using boost::optional;
using namespace dcp;

SubtitleAsset::SubtitleAsset (boost::filesystem::path file)
	: Content (file)
	, _need_sort (false)
{
	shared_ptr<cxml::Document> xml (new cxml::Document ("DCSubtitle"));
	xml->read_file (file);
	
	_id = xml->string_child ("SubtitleID");
	_movie_title = xml->string_child ("MovieTitle");
	_reel_number = xml->string_child ("ReelNumber");
	_language = xml->string_child ("Language");

	xml->ignore_child ("LoadFont");

	list<shared_ptr<dcp::parse::Font> > font_nodes = type_children<dcp::parse::Font> (xml, "Font");
	_load_font_nodes = type_children<dcp::parse::LoadFont> (xml, "LoadFont");

	/* Now make Subtitle objects to represent the raw XML nodes
	   in a sane way.
	*/

	ParseState parse_state;
	examine_font_nodes (xml, font_nodes, parse_state);
}

SubtitleAsset::SubtitleAsset (string directory, string movie_title, string language)
	: Content (directory)
	, _movie_title (movie_title)
	, _reel_number ("1")
	, _language (language)
	, _need_sort (false)
{

}

void
SubtitleAsset::examine_font_nodes (
	shared_ptr<const cxml::Node> xml,
	list<shared_ptr<dcp::parse::Font> > const & font_nodes,
	ParseState& parse_state
	)
{
	for (list<shared_ptr<dcp::parse::Font> >::const_iterator i = font_nodes.begin(); i != font_nodes.end(); ++i) {

		parse_state.font_nodes.push_back (*i);
		maybe_add_subtitle ((*i)->text, parse_state);

		for (list<shared_ptr<dcp::parse::Subtitle> >::iterator j = (*i)->subtitle_nodes.begin(); j != (*i)->subtitle_nodes.end(); ++j) {
			parse_state.subtitle_nodes.push_back (*j);
			examine_text_nodes (xml, (*j)->text_nodes, parse_state);
			examine_font_nodes (xml, (*j)->font_nodes, parse_state);
			parse_state.subtitle_nodes.pop_back ();
		}
	
		examine_font_nodes (xml, (*i)->font_nodes, parse_state);
		examine_text_nodes (xml, (*i)->text_nodes, parse_state);
		
		parse_state.font_nodes.pop_back ();
	}
}

void
SubtitleAsset::examine_text_nodes (
	shared_ptr<const cxml::Node> xml,
	list<shared_ptr<dcp::parse::Text> > const & text_nodes,
	ParseState& parse_state
	)
{
	for (list<shared_ptr<dcp::parse::Text> >::const_iterator i = text_nodes.begin(); i != text_nodes.end(); ++i) {
		parse_state.text_nodes.push_back (*i);
		maybe_add_subtitle ((*i)->text, parse_state);
		examine_font_nodes (xml, (*i)->font_nodes, parse_state);
		parse_state.text_nodes.pop_back ();
	}
}

void
SubtitleAsset::maybe_add_subtitle (string text, ParseState const & parse_state)
{
	if (empty_or_white_space (text)) {
		return;
	}
	
	if (parse_state.text_nodes.empty() || parse_state.subtitle_nodes.empty ()) {
		return;
	}

	assert (!parse_state.text_nodes.empty ());
	assert (!parse_state.subtitle_nodes.empty ());
	
	dcp::parse::Font effective_font (parse_state.font_nodes);
	dcp::parse::Text effective_text (*parse_state.text_nodes.back ());
	dcp::parse::Subtitle effective_subtitle (*parse_state.subtitle_nodes.back ());

	_subtitles.push_back (
		shared_ptr<Subtitle> (
			new Subtitle (
				font_id_to_name (effective_font.id),
				effective_font.italic.get(),
				effective_font.color.get(),
				effective_font.size,
				effective_subtitle.in,
				effective_subtitle.out,
				effective_text.v_position,
				effective_text.v_align,
				text,
				effective_font.effect ? effective_font.effect.get() : NONE,
				effective_font.effect_color.get(),
				effective_subtitle.fade_up_time,
				effective_subtitle.fade_down_time
				)
			)
		);
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
SubtitleAsset::font_id_to_name (string id) const
{
	list<shared_ptr<dcp::parse::LoadFont> >::const_iterator i = _load_font_nodes.begin();
	while (i != _load_font_nodes.end() && (*i)->id != id) {
		++i;
	}

	if (i == _load_font_nodes.end ()) {
		return "";
	}

	if ((*i)->uri == "arial.ttf") {
		return "Arial";
	}

	return "";
}

Subtitle::Subtitle (
	string font,
	bool italic,
	Color color,
	int size,
	Time in,
	Time out,
	float v_position,
	VAlign v_align,
	string text,
	Effect effect,
	Color effect_color,
	Time fade_up_time,
	Time fade_down_time
	)
	: _font (font)
	, _italic (italic)
	, _color (color)
	, _size (size)
	, _in (in)
	, _out (out)
	, _v_position (v_position)
	, _v_align (v_align)
	, _text (text)
	, _effect (effect)
	, _effect_color (effect_color)
	, _fade_up_time (fade_up_time)
	, _fade_down_time (fade_down_time)
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

bool
dcp::operator== (Subtitle const & a, Subtitle const & b)
{
	return (
		a.font() == b.font() &&
		a.italic() == b.italic() &&
		a.color() == b.color() &&
		a.size() == b.size() &&
		a.in() == b.in() &&
		a.out() == b.out() &&
		a.v_position() == b.v_position() &&
		a.v_align() == b.v_align() &&
		a.text() == b.text() &&
		a.effect() == b.effect() &&
		a.effect_color() == b.effect_color() &&
		a.fade_up_time() == b.fade_up_time() &&
		a.fade_down_time() == b.fade_down_time()
		);
}

ostream&
dcp::operator<< (ostream& s, Subtitle const & sub)
{
	s << "\n`" << sub.text() << "' from " << sub.in() << " to " << sub.out() << ";\n"
	  << "fade up " << sub.fade_up_time() << ", fade down " << sub.fade_down_time() << ";\n"
	  << "font " << sub.font() << ", ";

	if (sub.italic()) {
		s << "italic";
	} else {
		s << "non-italic";
	}
	
	s << ", size " << sub.size() << ", color " << sub.color() << ", vpos " << sub.v_position() << ", valign " << ((int) sub.v_align()) << ";\n"
	  << "effect " << ((int) sub.effect()) << ", effect color " << sub.effect_color();

	return s;
}

void
SubtitleAsset::add (shared_ptr<Subtitle> s)
{
	_subtitles.push_back (s);
	_need_sort = true;
}

void
SubtitleAsset::write_to_cpl (xmlpp::Element* node) const
{
	/* XXX: should EditRate, Duration and IntrinsicDuration be in here? */

	xmlpp::Node* ms = node->add_child ("MainSubtitle");
	ms->add_child("Id")->add_child_text("urn:uuid:" + _id);
	ms->add_child("AnnotationText")->add_child_text (_file.string ());
	/* XXX */
	ms->add_child("EntryPoint")->add_child_text ("0");
}

struct SubtitleSorter {
	bool operator() (shared_ptr<Subtitle> a, shared_ptr<Subtitle> b) {
		if (a->in() != b->in()) {
			return a->in() < b->in();
		}
		return a->v_position() < b->v_position();
	}
};

void
SubtitleAsset::write_xml () const
{
	FILE* f = fopen_boost (file (), "r");
	Glib::ustring const s = xml_as_string ();
	fwrite (s.c_str(), 1, s.length(), f);
	fclose (f);
}

Glib::ustring
SubtitleAsset::xml_as_string () const
{
	xmlpp::Document doc;
	xmlpp::Element* root = doc.create_root_node ("DCSubtitle");
	root->set_attribute ("Version", "1.0");

	root->add_child("SubtitleID")->add_child_text (_id);
	root->add_child("MovieTitle")->add_child_text (_movie_title);
	root->add_child("ReelNumber")->add_child_text (lexical_cast<string> (_reel_number));
	root->add_child("Language")->add_child_text (_language);

	if (_load_font_nodes.size() > 1) {
		boost::throw_exception (MiscError ("multiple LoadFont nodes not supported"));
	}

	if (!_load_font_nodes.empty ()) {
		xmlpp::Element* load_font = root->add_child("LoadFont");
		load_font->set_attribute("Id", _load_font_nodes.front()->id);
		load_font->set_attribute("URI",  _load_font_nodes.front()->uri);
	}

	list<shared_ptr<Subtitle> > sorted = _subtitles;
	if (_need_sort) {
		sorted.sort (SubtitleSorter ());
	}

	/* XXX: multiple fonts not supported */
	/* XXX: script, underlined, weight not supported */

	bool italic = false;
	Color color;
	int size = 0;
	Effect effect = NONE;
	Color effect_color;
	int spot_number = 1;
	Time last_in;
	Time last_out;
	Time last_fade_up_time;
	Time last_fade_down_time;

	xmlpp::Element* font = 0;
	xmlpp::Element* subtitle = 0;

	for (list<shared_ptr<Subtitle> >::iterator i = sorted.begin(); i != sorted.end(); ++i) {

		/* We will start a new <Font>...</Font> whenever some font property changes.
		   I suppose we should really make an optimal hierarchy of <Font> tags, but
		   that seems hard.
		*/

		bool const font_changed =
			italic       != (*i)->italic()       ||
			color        != (*i)->color()        ||
			size         != (*i)->size()         ||
			effect       != (*i)->effect()       ||
			effect_color != (*i)->effect_color();

		if (font_changed) {
			italic = (*i)->italic ();
			color = (*i)->color ();
			size = (*i)->size ();
			effect = (*i)->effect ();
			effect_color = (*i)->effect_color ();
		}

		if (!font || font_changed) {
			font = root->add_child ("Font");
			string id = "theFontId";
			if (!_load_font_nodes.empty()) {
				id = _load_font_nodes.front()->id;
			}
			font->set_attribute ("Id", id);
			font->set_attribute ("Italic", italic ? "yes" : "no");
			font->set_attribute ("Color", color.to_argb_string());
			font->set_attribute ("Size", lexical_cast<string> (size));
			font->set_attribute ("Effect", effect_to_string (effect));
			font->set_attribute ("EffectColor", effect_color.to_argb_string());
			font->set_attribute ("Script", "normal");
			font->set_attribute ("Underlined", "no");
			font->set_attribute ("Weight", "normal");
		}

		if (!subtitle || font_changed ||
		    (last_in != (*i)->in() ||
		     last_out != (*i)->out() ||
		     last_fade_up_time != (*i)->fade_up_time() ||
		     last_fade_down_time != (*i)->fade_down_time()
			    )) {

			subtitle = font->add_child ("Subtitle");
			subtitle->set_attribute ("SpotNumber", lexical_cast<string> (spot_number++));
			subtitle->set_attribute ("TimeIn", (*i)->in().to_string());
			subtitle->set_attribute ("TimeOut", (*i)->out().to_string());
			subtitle->set_attribute ("FadeUpTime", lexical_cast<string> ((*i)->fade_up_time().to_ticks()));
			subtitle->set_attribute ("FadeDownTime", lexical_cast<string> ((*i)->fade_down_time().to_ticks()));

			last_in = (*i)->in ();
			last_out = (*i)->out ();
			last_fade_up_time = (*i)->fade_up_time ();
			last_fade_down_time = (*i)->fade_down_time ();
		}

		xmlpp::Element* text = subtitle->add_child ("Text");
		text->set_attribute ("VAlign", valign_to_string ((*i)->v_align()));		
		text->set_attribute ("VPosition", lexical_cast<string> ((*i)->v_position()));
		text->add_child_text ((*i)->text());
	}

	return doc.write_to_string_formatted ("UTF-8");
}

