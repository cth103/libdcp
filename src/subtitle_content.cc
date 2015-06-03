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

#include "raw_convert.h"
#include "subtitle_content.h"
#include "util.h"
#include "xml.h"
#include "font_node.h"
#include "text_node.h"
#include "subtitle_string.h"
#include "dcp_assert.h"
#include "AS_DCP.h"
#include "KM_util.h"
#include <libxml++/nodes/element.h>
#include <boost/algorithm/string.hpp>
#include <fstream>

using std::string;
using std::list;
using std::ostream;
using std::ofstream;
using std::stringstream;
using std::cout;
using boost::shared_ptr;
using boost::optional;
using boost::dynamic_pointer_cast;
using namespace dcp;

SubtitleContent::SubtitleContent ()
	: _reel_number ("1")
{

}

SubtitleContent::SubtitleContent (boost::filesystem::path file)
	: Content (file)
	, _reel_number ("1")
{

}

void
SubtitleContent::parse_common (shared_ptr<cxml::Document> xml, list<shared_ptr<dcp::FontNode> > font_nodes)
{
	_reel_number = xml->string_child ("ReelNumber");
	_language = xml->string_child ("Language");

	/* Now make Subtitle objects to represent the raw XML nodes
	   in a sane way.
	*/

	ParseState parse_state;
	examine_font_nodes (xml, font_nodes, parse_state);
}

void
SubtitleContent::examine_font_nodes (
	shared_ptr<const cxml::Node> xml,
	list<shared_ptr<dcp::FontNode> > const & font_nodes,
	ParseState& parse_state
	)
{
	for (list<shared_ptr<dcp::FontNode> >::const_iterator i = font_nodes.begin(); i != font_nodes.end(); ++i) {

		parse_state.font_nodes.push_back (*i);
		maybe_add_subtitle ((*i)->text, parse_state);

		for (list<shared_ptr<dcp::SubtitleNode> >::iterator j = (*i)->subtitle_nodes.begin(); j != (*i)->subtitle_nodes.end(); ++j) {
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
SubtitleContent::examine_text_nodes (
	shared_ptr<const cxml::Node> xml,
	list<shared_ptr<dcp::TextNode> > const & text_nodes,
	ParseState& parse_state
	)
{
	for (list<shared_ptr<dcp::TextNode> >::const_iterator i = text_nodes.begin(); i != text_nodes.end(); ++i) {
		parse_state.text_nodes.push_back (*i);
		maybe_add_subtitle ((*i)->text, parse_state);
		examine_font_nodes (xml, (*i)->font_nodes, parse_state);
		parse_state.text_nodes.pop_back ();
	}
}

void
SubtitleContent::maybe_add_subtitle (string text, ParseState const & parse_state)
{
	if (empty_or_white_space (text)) {
		return;
	}
	
	if (parse_state.text_nodes.empty() || parse_state.subtitle_nodes.empty ()) {
		return;
	}

	DCP_ASSERT (!parse_state.text_nodes.empty ());
	DCP_ASSERT (!parse_state.subtitle_nodes.empty ());
	
	dcp::FontNode effective_font (parse_state.font_nodes);
	dcp::TextNode effective_text (*parse_state.text_nodes.back ());
	dcp::SubtitleNode effective_subtitle (*parse_state.subtitle_nodes.back ());

	_subtitles.push_back (
		SubtitleString (
			effective_font.id,
			effective_font.italic.get_value_or (false),
			effective_font.colour.get_value_or (dcp::Colour (255, 255, 255)),
			effective_font.size,
			effective_font.aspect_adjust.get_value_or (1.0),
			effective_subtitle.in,
			effective_subtitle.out,
			effective_text.h_position,
			effective_text.h_align,
			effective_text.v_position,
			effective_text.v_align,
			text,
			effective_font.effect.get_value_or (NONE),
			effective_font.effect_colour.get_value_or (dcp::Colour (0, 0, 0)),
			effective_subtitle.fade_up_time,
			effective_subtitle.fade_down_time
			)
		);
}

list<SubtitleString>
SubtitleContent::subtitles_during (Time from, Time to) const
{
	list<SubtitleString> s;
	for (list<SubtitleString>::const_iterator i = _subtitles.begin(); i != _subtitles.end(); ++i) {
		if (i->out() >= from && i->in() <= to) {
			s.push_back (*i);
		}
	}

	return s;
}

void
SubtitleContent::add (SubtitleString s)
{
	_subtitles.push_back (s);
}

void
SubtitleContent::write_xml (boost::filesystem::path p) const
{
	FILE* f = fopen_boost (p, "w");
	if (!f) {
		throw FileError ("Could not open file for writing", p, -1);
	}
	
	Glib::ustring const s = xml_as_string ();
	fwrite (s.c_str(), 1, s.bytes(), f);
	fclose (f);

	_file = p;
}

Time
SubtitleContent::latest_subtitle_out () const
{
	Time t;
	for (list<SubtitleString>::const_iterator i = _subtitles.begin(); i != _subtitles.end(); ++i) {
		if (i->out() > t) {
			t = i->out ();
		}
	}

	return t;
}

bool
SubtitleContent::equals (shared_ptr<const Asset> other_asset, EqualityOptions options, NoteHandler note) const
{
	if (!Asset::equals (other_asset, options, note)) {
		return false;
	}
	
	shared_ptr<const SubtitleContent> other = dynamic_pointer_cast<const SubtitleContent> (other_asset);
	if (!other) {
		return false;
	}

	if (_reel_number != other->_reel_number) {
		note (DCP_ERROR, "subtitle reel numbers differ");
		return false;
	}

	if (_language != other->_language) {
		note (DCP_ERROR, "subtitle languages differ");
		return false;
	}

	if (_subtitles != other->_subtitles) {
		note (DCP_ERROR, "subtitles differ");
		return false;
	}

	return true;
}
