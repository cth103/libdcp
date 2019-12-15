/*
    Copyright (C) 2012-2019 Carl Hetherington <cth@carlh.net>

    This file is part of libdcp.

    libdcp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libdcp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdcp.  If not, see <http://www.gnu.org/licenses/>.

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/

#include "raw_convert.h"
#include "compose.hpp"
#include "subtitle_asset.h"
#include "subtitle_asset_internal.h"
#include "util.h"
#include "xml.h"
#include "subtitle_string.h"
#include "subtitle_image.h"
#include "dcp_assert.h"
#include "load_font_node.h"
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_util.h>
#include <libxml++/nodes/element.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_array.hpp>
#include <boost/foreach.hpp>

using std::string;
using std::list;
using std::cout;
using std::cerr;
using std::map;
using boost::shared_ptr;
using boost::shared_array;
using boost::optional;
using boost::dynamic_pointer_cast;
using boost::lexical_cast;
using namespace dcp;

SubtitleAsset::SubtitleAsset ()
{

}

SubtitleAsset::SubtitleAsset (boost::filesystem::path file)
	: Asset (file)
{

}

string
string_attribute (xmlpp::Element const * node, string name)
{
	xmlpp::Attribute* a = node->get_attribute (name);
	if (!a) {
		throw XMLError (String::compose ("missing attribute %1", name));
	}
	return string (a->get_value ());
}

optional<string>
optional_string_attribute (xmlpp::Element const * node, string name)
{
	xmlpp::Attribute* a = node->get_attribute (name);
	if (!a) {
		return optional<string>();
	}
	return string (a->get_value ());
}

optional<bool>
optional_bool_attribute (xmlpp::Element const * node, string name)
{
	optional<string> s = optional_string_attribute (node, name);
	if (!s) {
		return optional<bool> ();
	}

	return (s.get() == "1" || s.get() == "yes");
}

template <class T>
optional<T>
optional_number_attribute (xmlpp::Element const * node, string name)
{
	boost::optional<std::string> s = optional_string_attribute (node, name);
	if (!s) {
		return boost::optional<T> ();
	}

	std::string t = s.get ();
	boost::erase_all (t, " ");
	return raw_convert<T> (t);
}

SubtitleAsset::ParseState
SubtitleAsset::font_node_state (xmlpp::Element const * node, Standard standard) const
{
	ParseState ps;

	if (standard == INTEROP) {
		ps.font_id = optional_string_attribute (node, "Id");
	} else {
		ps.font_id = optional_string_attribute (node, "ID");
	}
	ps.size = optional_number_attribute<int64_t> (node, "Size");
	ps.aspect_adjust = optional_number_attribute<float> (node, "AspectAdjust");
	ps.italic = optional_bool_attribute (node, "Italic");
	ps.bold = optional_string_attribute(node, "Weight").get_value_or("normal") == "bold";
	if (standard == INTEROP) {
		ps.underline = optional_bool_attribute (node, "Underlined");
	} else {
		ps.underline = optional_bool_attribute (node, "Underline");
	}
	optional<string> c = optional_string_attribute (node, "Color");
	if (c) {
		ps.colour = Colour (c.get ());
	}
	optional<string> const e = optional_string_attribute (node, "Effect");
	if (e) {
		ps.effect = string_to_effect (e.get ());
	}
	c = optional_string_attribute (node, "EffectColor");
	if (c) {
		ps.effect_colour = Colour (c.get ());
	}

	return ps;
}

void
SubtitleAsset::position_align (SubtitleAsset::ParseState& ps, xmlpp::Element const * node) const
{
	optional<float> hp = optional_number_attribute<float> (node, "HPosition");
	if (!hp) {
		hp = optional_number_attribute<float> (node, "Hposition");
	}
	if (hp) {
		ps.h_position = hp.get () / 100;
	}

	optional<string> ha = optional_string_attribute (node, "HAlign");
	if (!ha) {
		ha = optional_string_attribute (node, "Halign");
	}
	if (ha) {
		ps.h_align = string_to_halign (ha.get ());
	}

	optional<float> vp = optional_number_attribute<float> (node, "VPosition");
	if (!vp) {
		vp = optional_number_attribute<float> (node, "Vposition");
	}
	if (vp) {
		ps.v_position = vp.get () / 100;
	}

	optional<string> va = optional_string_attribute (node, "VAlign");
	if (!va) {
		va = optional_string_attribute (node, "Valign");
	}
	if (va) {
		ps.v_align = string_to_valign (va.get ());
	}

}

SubtitleAsset::ParseState
SubtitleAsset::text_node_state (xmlpp::Element const * node) const
{
	ParseState ps;

	position_align (ps, node);

	optional<string> d = optional_string_attribute (node, "Direction");
	if (d) {
		ps.direction = string_to_direction (d.get ());
	}

	ps.type = ParseState::TEXT;

	return ps;
}

SubtitleAsset::ParseState
SubtitleAsset::image_node_state (xmlpp::Element const * node) const
{
	ParseState ps;

	position_align (ps, node);

	ps.type = ParseState::IMAGE;

	return ps;
}

SubtitleAsset::ParseState
SubtitleAsset::subtitle_node_state (xmlpp::Element const * node, optional<int> tcr) const
{
	ParseState ps;
	ps.in = Time (string_attribute(node, "TimeIn"), tcr);
	ps.out = Time (string_attribute(node, "TimeOut"), tcr);
	ps.fade_up_time = fade_time (node, "FadeUpTime", tcr);
	ps.fade_down_time = fade_time (node, "FadeDownTime", tcr);
	return ps;
}

Time
SubtitleAsset::fade_time (xmlpp::Element const * node, string name, optional<int> tcr) const
{
	string const u = optional_string_attribute(node, name).get_value_or ("");
	Time t;

	if (u.empty ()) {
		t = Time (0, 0, 0, 20, 250);
	} else if (u.find (":") != string::npos) {
		t = Time (u, tcr);
	} else {
		t = Time (0, 0, 0, lexical_cast<int> (u), tcr.get_value_or(250));
	}

	if (t > Time (0, 0, 8, 0, 250)) {
		t = Time (0, 0, 8, 0, 250);
	}

	return t;
}

void
SubtitleAsset::parse_subtitles (xmlpp::Element const * node, list<ParseState>& state, optional<int> tcr, Standard standard)
{
	if (node->get_name() == "Font") {
		state.push_back (font_node_state (node, standard));
	} else if (node->get_name() == "Subtitle") {
		state.push_back (subtitle_node_state (node, tcr));
	} else if (node->get_name() == "Text") {
		state.push_back (text_node_state (node));
	} else if (node->get_name() == "SubtitleList") {
		state.push_back (ParseState ());
	} else if (node->get_name() == "Image") {
		state.push_back (image_node_state (node));
	} else {
		throw XMLError ("unexpected node " + node->get_name());
	}

	xmlpp::Node::NodeList c = node->get_children ();
	for (xmlpp::Node::NodeList::const_iterator i = c.begin(); i != c.end(); ++i) {
		xmlpp::ContentNode const * v = dynamic_cast<xmlpp::ContentNode const *> (*i);
		if (v) {
			maybe_add_subtitle (v->get_content(), state, standard);
		}
		xmlpp::Element const * e = dynamic_cast<xmlpp::Element const *> (*i);
		if (e) {
			parse_subtitles (e, state, tcr, standard);
		}
	}

	state.pop_back ();
}

void
SubtitleAsset::maybe_add_subtitle (string text, list<ParseState> const & parse_state, Standard standard)
{
	if (empty_or_white_space (text)) {
		return;
	}

	ParseState ps;
	BOOST_FOREACH (ParseState const & i, parse_state) {
		if (i.font_id) {
			ps.font_id = i.font_id.get();
		}
		if (i.size) {
			ps.size = i.size.get();
		}
		if (i.aspect_adjust) {
			ps.aspect_adjust = i.aspect_adjust.get();
		}
		if (i.italic) {
			ps.italic = i.italic.get();
		}
		if (i.bold) {
			ps.bold = i.bold.get();
		}
		if (i.underline) {
			ps.underline = i.underline.get();
		}
		if (i.colour) {
			ps.colour = i.colour.get();
		}
		if (i.effect) {
			ps.effect = i.effect.get();
		}
		if (i.effect_colour) {
			ps.effect_colour = i.effect_colour.get();
		}
		if (i.h_position) {
			ps.h_position = i.h_position.get();
		}
		if (i.h_align) {
			ps.h_align = i.h_align.get();
		}
		if (i.v_position) {
			ps.v_position = i.v_position.get();
		}
		if (i.v_align) {
			ps.v_align = i.v_align.get();
		}
		if (i.direction) {
			ps.direction = i.direction.get();
		}
		if (i.in) {
			ps.in = i.in.get();
		}
		if (i.out) {
			ps.out = i.out.get();
		}
		if (i.fade_up_time) {
			ps.fade_up_time = i.fade_up_time.get();
		}
		if (i.fade_down_time) {
			ps.fade_down_time = i.fade_down_time.get();
		}
		if (i.type) {
			ps.type = i.type.get();
		}
	}

	if (!ps.in || !ps.out) {
		/* We're not in a <Subtitle> node; just ignore this content */
		return;
	}

	DCP_ASSERT (ps.type);

	switch (ps.type.get()) {
	case ParseState::TEXT:
		_subtitles.push_back (
			shared_ptr<Subtitle> (
				new SubtitleString (
					ps.font_id,
					ps.italic.get_value_or (false),
					ps.bold.get_value_or (false),
					ps.underline.get_value_or (false),
					ps.colour.get_value_or (dcp::Colour (255, 255, 255)),
					ps.size.get_value_or (42),
					ps.aspect_adjust.get_value_or (1.0),
					ps.in.get(),
					ps.out.get(),
					ps.h_position.get_value_or(0),
					ps.h_align.get_value_or(HALIGN_CENTER),
					ps.v_position.get_value_or(0),
					ps.v_align.get_value_or(VALIGN_CENTER),
					ps.direction.get_value_or (DIRECTION_LTR),
					text,
					ps.effect.get_value_or (NONE),
					ps.effect_colour.get_value_or (dcp::Colour (0, 0, 0)),
					ps.fade_up_time.get_value_or(Time()),
					ps.fade_down_time.get_value_or(Time())
					)
				)
			);
		break;
	case ParseState::IMAGE:
		/* Add a subtitle with no image data and we'll fill that in later */
		_subtitles.push_back (
			shared_ptr<Subtitle> (
				new SubtitleImage (
					Data (),
					standard == INTEROP ? text.substr(0, text.size() - 4) : text,
					ps.in.get(),
					ps.out.get(),
					ps.h_position.get_value_or(0),
					ps.h_align.get_value_or(HALIGN_CENTER),
					ps.v_position.get_value_or(0),
					ps.v_align.get_value_or(VALIGN_CENTER),
					ps.fade_up_time.get_value_or(Time()),
					ps.fade_down_time.get_value_or(Time())
					)
				)
			);
		break;
	}
}

list<shared_ptr<Subtitle> >
SubtitleAsset::subtitles_during (Time from, Time to, bool starting) const
{
	list<shared_ptr<Subtitle> > s;
	BOOST_FOREACH (shared_ptr<Subtitle> i, _subtitles) {
		if ((starting && from <= i->in() && i->in() < to) || (!starting && i->out() >= from && i->in() <= to)) {
			s.push_back (i);
		}
	}

	return s;
}

void
SubtitleAsset::add (shared_ptr<Subtitle> s)
{
	_subtitles.push_back (s);
}

Time
SubtitleAsset::latest_subtitle_out () const
{
	Time t;
	BOOST_FOREACH (shared_ptr<Subtitle> i, _subtitles) {
		if (i->out() > t) {
			t = i->out ();
		}
	}

	return t;
}

bool
SubtitleAsset::equals (shared_ptr<const Asset> other_asset, EqualityOptions options, NoteHandler note) const
{
	if (!Asset::equals (other_asset, options, note)) {
		return false;
	}

	shared_ptr<const SubtitleAsset> other = dynamic_pointer_cast<const SubtitleAsset> (other_asset);
	if (!other) {
		return false;
	}

	if (_subtitles.size() != other->_subtitles.size()) {
		note (DCP_ERROR, "subtitles differ");
		return false;
	}

	list<shared_ptr<Subtitle> >::const_iterator i = _subtitles.begin ();
	list<shared_ptr<Subtitle> >::const_iterator j = other->_subtitles.begin ();

	while (i != _subtitles.end()) {
		shared_ptr<SubtitleString> string_i = dynamic_pointer_cast<SubtitleString> (*i);
		shared_ptr<SubtitleString> string_j = dynamic_pointer_cast<SubtitleString> (*j);
		shared_ptr<SubtitleImage> image_i = dynamic_pointer_cast<SubtitleImage> (*i);
		shared_ptr<SubtitleImage> image_j = dynamic_pointer_cast<SubtitleImage> (*j);

		if ((string_i && !string_j) || (image_i && !image_j)) {
			note (DCP_ERROR, "subtitles differ");
			return false;
		}

		if (string_i && *string_i != *string_j) {
			note (DCP_ERROR, "subtitles differ");
			return false;
		}

		if (image_i && *image_i != *image_j) {
			note (DCP_ERROR, "subtitles differ");
			return false;
		}

		++i;
		++j;
	}

	return true;
}

struct SubtitleSorter
{
	bool operator() (shared_ptr<Subtitle> a, shared_ptr<Subtitle> b) {
		if (a->in() != b->in()) {
			return a->in() < b->in();
		}
		return a->v_position() < b->v_position();
	}
};

void
SubtitleAsset::pull_fonts (shared_ptr<order::Part> part)
{
	if (part->children.empty ()) {
		return;
	}

	/* Pull up from children */
	BOOST_FOREACH (shared_ptr<order::Part> i, part->children) {
		pull_fonts (i);
	}

	if (part->parent) {
		/* Establish the common font features that each of part's children have;
		   these features go into part's font.
		*/
		part->font = part->children.front()->font;
		BOOST_FOREACH (shared_ptr<order::Part> i, part->children) {
			part->font.take_intersection (i->font);
		}

		/* Remove common values from part's children's fonts */
		BOOST_FOREACH (shared_ptr<order::Part> i, part->children) {
			i->font.take_difference (part->font);
		}
	}

	/* Merge adjacent children with the same font */
	list<shared_ptr<order::Part> >::const_iterator i = part->children.begin();
	list<shared_ptr<order::Part> > merged;

	while (i != part->children.end()) {

		if ((*i)->font.empty ()) {
			merged.push_back (*i);
			++i;
		} else {
			list<shared_ptr<order::Part> >::const_iterator j = i;
			++j;
			while (j != part->children.end() && (*i)->font == (*j)->font) {
				++j;
			}
			if (std::distance (i, j) == 1) {
				merged.push_back (*i);
				++i;
			} else {
				shared_ptr<order::Part> group (new order::Part (part, (*i)->font));
				for (list<shared_ptr<order::Part> >::const_iterator k = i; k != j; ++k) {
					(*k)->font.clear ();
					group->children.push_back (*k);
				}
				merged.push_back (group);
				i = j;
			}
		}
	}

	part->children = merged;
}

/** @param standard Standard (INTEROP or SMPTE); this is used rather than putting things in the child
 *  class because the differences between the two are fairly subtle.
 */
void
SubtitleAsset::subtitles_as_xml (xmlpp::Element* xml_root, int time_code_rate, Standard standard) const
{
	list<shared_ptr<Subtitle> > sorted = _subtitles;
	sorted.sort (SubtitleSorter ());

	/* Gather our subtitles into a hierarchy of Subtitle/Text/String objects, writing
	   font information into the bottom level (String) objects.
	*/

	shared_ptr<order::Part> root (new order::Part (shared_ptr<order::Part> ()));
	shared_ptr<order::Subtitle> subtitle;
	shared_ptr<order::Text> text;

	Time last_in;
	Time last_out;
	Time last_fade_up_time;
	Time last_fade_down_time;
	HAlign last_h_align;
	float last_h_position;
	VAlign last_v_align;
	float last_v_position;
	Direction last_direction;

	BOOST_FOREACH (shared_ptr<Subtitle> i, sorted) {
		if (!subtitle ||
		    (last_in != i->in() ||
		     last_out != i->out() ||
		     last_fade_up_time != i->fade_up_time() ||
		     last_fade_down_time != i->fade_down_time())
			) {

			subtitle.reset (new order::Subtitle (root, i->in(), i->out(), i->fade_up_time(), i->fade_down_time()));
			root->children.push_back (subtitle);

			last_in = i->in ();
			last_out = i->out ();
			last_fade_up_time = i->fade_up_time ();
			last_fade_down_time = i->fade_down_time ();
			text.reset ();
		}

		shared_ptr<SubtitleString> is = dynamic_pointer_cast<SubtitleString>(i);
		if (is) {
			if (!text ||
			    last_h_align != is->h_align() ||
			    fabs(last_h_position - is->h_position()) > ALIGN_EPSILON ||
			    last_v_align != is->v_align() ||
			    fabs(last_v_position - is->v_position()) > ALIGN_EPSILON ||
			    last_direction != is->direction()
				) {
				text.reset (new order::Text (subtitle, is->h_align(), is->h_position(), is->v_align(), is->v_position(), is->direction()));
				subtitle->children.push_back (text);

				last_h_align = is->h_align ();
				last_h_position = is->h_position ();
				last_v_align = is->v_align ();
				last_v_position = is->v_position ();
				last_direction = is->direction ();
			}

			text->children.push_back (shared_ptr<order::String> (new order::String (text, order::Font (is, standard), is->text())));
		}

		shared_ptr<SubtitleImage> ii = dynamic_pointer_cast<SubtitleImage>(i);
		if (ii) {
			text.reset ();
			subtitle->children.push_back (
				shared_ptr<order::Image> (new order::Image (subtitle, ii->id(), ii->png_image(), ii->h_align(), ii->h_position(), ii->v_align(), ii->v_position()))
				);
		}
	}

	/* Pull font changes as high up the hierarchy as we can */

	pull_fonts (root);

	/* Write XML */

	order::Context context;
	context.time_code_rate = time_code_rate;
	context.standard = standard;
	context.spot_number = 1;

	root->write_xml (xml_root, context);
}

map<string, Data>
SubtitleAsset::fonts_with_load_ids () const
{
	map<string, Data> out;
	BOOST_FOREACH (Font const & i, _fonts) {
		out[i.load_id] = i.data;
	}
	return out;
}

/** Replace empty IDs in any <LoadFontId> and <Font> tags with
 *  a dummy string.  Some systems give errors with empty font IDs
 *  (see DCP-o-matic bug #1689).
 */
void
SubtitleAsset::fix_empty_font_ids ()
{
	bool have_empty = false;
	list<string> ids;
	BOOST_FOREACH (shared_ptr<LoadFontNode> i, load_font_nodes()) {
		if (i->id == "") {
			have_empty = true;
		} else {
			ids.push_back (i->id);
		}
	}

	if (!have_empty) {
		return;
	}

	string const empty_id = unique_string (ids, "font");

	BOOST_FOREACH (shared_ptr<LoadFontNode> i, load_font_nodes()) {
		if (i->id == "") {
			i->id = empty_id;
		}
	}

	BOOST_FOREACH (shared_ptr<Subtitle> i, _subtitles) {
		shared_ptr<SubtitleString> j = dynamic_pointer_cast<SubtitleString> (i);
		if (j && j->font() && j->font().get() == "") {
			j->set_font (empty_id);
		}
	}
}
