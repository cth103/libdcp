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
#include "subtitle_asset.h"
#include "util.h"

using std::string;
using std::list;
using std::ostream;
using std::ofstream;
using std::stringstream;
using boost::shared_ptr;
using boost::lexical_cast;
using namespace libdcp;

SubtitleAsset::SubtitleAsset (string directory, string xml_file)
	: Asset (directory, xml_file)
{
	read_xml (path().string());
}

SubtitleAsset::SubtitleAsset (string directory, string movie_title, string language)
	: Asset (directory)
	, _movie_title (movie_title)
	, _reel_number ("1")
	, _language (language)
{

}

void
SubtitleAsset::read_xml (string xml_file)
{
	shared_ptr<XMLFile> xml (new XMLFile (xml_file, "DCSubtitle"));
	
	_uuid = xml->string_child ("SubtitleID");
	_movie_title = xml->string_child ("MovieTitle");
	_reel_number = xml->string_child ("ReelNumber");
	_language = xml->string_child ("Language");

	xml->ignore_child ("LoadFont");

	list<shared_ptr<FontNode> > font_nodes = xml->type_children<FontNode> ("Font");
	_load_font_nodes = xml->type_children<LoadFontNode> ("LoadFont");

	/* Now make Subtitle objects to represent the raw XML nodes
	   in a sane way.
	*/

	ParseState parse_state;
	examine_font_nodes (xml, font_nodes, parse_state);
}

void
SubtitleAsset::examine_font_nodes (
	shared_ptr<XMLFile> xml,
	list<shared_ptr<FontNode> > const & font_nodes,
	ParseState& parse_state
	)
{
	for (list<shared_ptr<FontNode> >::const_iterator i = font_nodes.begin(); i != font_nodes.end(); ++i) {

		parse_state.font_nodes.push_back (*i);
		maybe_add_subtitle ((*i)->text, parse_state);

		for (list<shared_ptr<SubtitleNode> >::iterator j = (*i)->subtitle_nodes.begin(); j != (*i)->subtitle_nodes.end(); ++j) {
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
	shared_ptr<XMLFile> xml,
	list<shared_ptr<TextNode> > const & text_nodes,
	ParseState& parse_state
	)
{
	for (list<shared_ptr<TextNode> >::const_iterator i = text_nodes.begin(); i != text_nodes.end(); ++i) {
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
	
	FontNode effective_font (parse_state.font_nodes);
	TextNode effective_text (*parse_state.text_nodes.back ());
	SubtitleNode effective_subtitle (*parse_state.subtitle_nodes.back ());

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

FontNode::FontNode (xmlpp::Node const * node)
	: XMLNode (node)
{
	text = content ();
	
	id = optional_string_attribute ("Id");
	size = optional_int64_attribute ("Size");
	italic = optional_bool_attribute ("Italic");
	color = optional_color_attribute ("Color");
	string const e = optional_string_attribute ("Effect");
	if (!e.empty ()) {
		effect = string_to_effect (e);
	}
	effect_color = optional_color_attribute ("EffectColor");
	subtitle_nodes = type_children<SubtitleNode> ("Subtitle");
	font_nodes = type_children<FontNode> ("Font");
	text_nodes = type_children<TextNode> ("Text");
}

FontNode::FontNode (list<shared_ptr<FontNode> > const & font_nodes)
	: size (0)
	, italic (false)
	, color ("FFFFFFFF")
	, effect_color ("FFFFFFFF")
{
	for (list<shared_ptr<FontNode> >::const_iterator i = font_nodes.begin(); i != font_nodes.end(); ++i) {
		if (!(*i)->id.empty ()) {
			id = (*i)->id;
		}
		if ((*i)->size != 0) {
			size = (*i)->size;
		}
		if ((*i)->italic) {
			italic = (*i)->italic.get ();
		}
		if ((*i)->color) {
			color = (*i)->color.get ();
		}
		if ((*i)->effect) {
			effect = (*i)->effect.get ();
		}
		if ((*i)->effect_color) {
			effect_color = (*i)->effect_color.get ();
		}
	}
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
	font_nodes = type_children<FontNode> ("Font");
	text_nodes = type_children<TextNode> ("Text");
	fade_up_time = fade_time ("FadeUpTime");
	fade_down_time = fade_time ("FadeDownTime");
}

Time
SubtitleNode::fade_time (string name)
{
	string const u = optional_string_attribute (name);
	Time t;
	
	if (u.empty ()) {
		t = Time (0, 0, 0, 20);
	} else if (u.find (":") != string::npos) {
		t = Time (u);
	} else {
		t = Time (0, 0, 0, lexical_cast<int> (u));
	}

	if (t > Time (0, 0, 8, 0)) {
		t = Time (0, 0, 8, 0);
	}

	return t;
}

TextNode::TextNode (xmlpp::Node const * node)
	: XMLNode (node)
	, v_align (CENTER)
{
	text = content ();
	v_position = float_attribute ("VPosition");
	string const v = optional_string_attribute ("VAlign");
	if (!v.empty ()) {
		v_align = string_to_valign (v);
	}

	font_nodes = type_children<FontNode> ("Font");
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
	list<shared_ptr<LoadFontNode> >::const_iterator i = _load_font_nodes.begin();
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
libdcp::operator== (Subtitle const & a, Subtitle const & b)
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
libdcp::operator<< (ostream& s, Subtitle const & sub)
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
}

void
SubtitleAsset::write_to_cpl (ostream& s) const
{
	/* XXX: should EditRate, Duration and IntrinsicDuration be in here? */
	
	s << "        <MainSubtitle>\n"
	  << "          <Id>urn:uuid:" << _uuid << "</Id>\n"
	  << "          <AnnotationText>" << _file_name << "</AnnotationText>\n"
	  << "          <EntryPoint>0</EntryPoint>\n"
	  << "        </MainSubtitle>\n";
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
SubtitleAsset::write_xml ()
{
	ofstream f (path().string().c_str());
	write_xml (f);
}

void
SubtitleAsset::write_xml (ostream& s)
{
	s << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	  << "<DCSubtitle Version=\"1.0\">\n"
	  << "  <SubtitleID>" << _uuid << "</SubtitleID>\n"
	  << "  <MovieTitle>" << _movie_title << "</MovieTitle>\n"
	  << "  <ReelNumber>" << _reel_number << "</ReelNumber>\n"
	  << "  <Language>" << _language << "</Language>\n"
	  << "  <LoadFont Id=\"theFontId\" URI=\"arial.ttf\"/>\n";

	_subtitles.sort (SubtitleSorter ());

	/* XXX: multiple fonts not supported */
	/* XXX: script, underlined, weight not supported */

	bool first = true;
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

	for (list<shared_ptr<Subtitle> >::iterator i = _subtitles.begin(); i != _subtitles.end(); ++i) {

		/* We will start a new <Font>...</Font> whenever some font property changes.
		   I suppose should really make an optimal hierarchy of <Font> tags, but
		   that seems hard.
		*/

		bool const font_changed = first              ||
			italic       != (*i)->italic()       ||
			color        != (*i)->color()        ||
			size         != (*i)->size()         ||
			effect       != (*i)->effect()       ||
			effect_color != (*i)->effect_color();

		stringstream a;
		if (font_changed) {
			italic = (*i)->italic ();
			a << "Italic=\"" << (italic ? "yes" : "no") << "\" ";
			color = (*i)->color ();
			a << "Color=\"" << color.to_argb_string() << "\" ";
			size = (*i)->size ();
			a << "Size=\"" << size << "\" ";
			effect = (*i)->effect ();
			a << "Effect=\"" << effect_to_string(effect) << "\" ";
			effect_color = (*i)->effect_color ();
			a << "EffectColor=\"" << effect_color.to_argb_string() << "\" ";
			a << "Script=\"normal\" Underlined=\"no\" Weight=\"normal\"";
		}

		if (first ||
		    (last_in != (*i)->in() ||
		     last_out != (*i)->out() ||
		     last_fade_up_time != (*i)->fade_up_time() ||
		     last_fade_down_time != (*i)->fade_down_time()
			    )) {

			if (!first) {
				s << "  </Subtitle>\n";
			}

			if (font_changed) {
				if (!first) {
					s << "  </Font>\n";
				}
				s << "  <Font Id=\"theFontId\" " << a.str() << ">\n";
			}

			s << "  <Subtitle "
			  << "SpotNumber=\"" << spot_number++ << "\" "
			  << "TimeIn=\"" << (*i)->in().to_string() << "\" "
			  << "TimeOut=\"" << (*i)->out().to_string() << "\" "
			  << "FadeUpTime=\"" << (*i)->fade_up_time().to_ticks() << "\" "
			  << "FadeDownTime=\"" << (*i)->fade_down_time().to_ticks() << "\""
			  << ">\n";

			last_in = (*i)->in ();
			last_out = (*i)->out ();
			last_fade_up_time = (*i)->fade_up_time ();
			last_fade_down_time = (*i)->fade_down_time ();
		}

		s << "      <Text "
		  << "VAlign=\"" << valign_to_string ((*i)->v_align()) << "\" "
		  << "VPosition=\"" << (*i)->v_position() << "\""
		  << ">" << (*i)->text() << "</Text>\n";

		first = false;
	}

	s << "  </Subtitle>\n";
	s << "  </Font>\n";
	s << "</DCSubtitle>\n";
}
