/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/subtitle_asset.cc
 *  @brief SubtitleAsset class
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
#include "reel_asset.h"
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_util.h>
#include <libxml++/nodes/element.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_array.hpp>


using std::dynamic_pointer_cast;
using std::string;
using std::cout;
using std::cerr;
using std::map;
using std::shared_ptr;
using std::vector;
using std::make_shared;
using boost::shared_array;
using boost::optional;
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
	auto a = node->get_attribute (name);
	if (!a) {
		throw XMLError (String::compose ("missing attribute %1", name));
	}
	return string (a->get_value ());
}


optional<string>
optional_string_attribute (xmlpp::Element const * node, string name)
{
	auto a = node->get_attribute (name);
	if (!a) {
		return {};
	}
	return string (a->get_value ());
}


optional<bool>
optional_bool_attribute (xmlpp::Element const * node, string name)
{
	auto s = optional_string_attribute (node, name);
	if (!s) {
		return {};
	}

	return (s.get() == "1" || s.get() == "yes");
}


template <class T>
optional<T>
optional_number_attribute (xmlpp::Element const * node, string name)
{
	auto s = optional_string_attribute (node, name);
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

	if (standard == Standard::INTEROP) {
		ps.font_id = optional_string_attribute (node, "Id");
	} else {
		ps.font_id = optional_string_attribute (node, "ID");
	}
	ps.size = optional_number_attribute<int64_t> (node, "Size");
	ps.aspect_adjust = optional_number_attribute<float> (node, "AspectAdjust");
	ps.italic = optional_bool_attribute (node, "Italic");
	ps.bold = optional_string_attribute(node, "Weight").get_value_or("normal") == "bold";
	if (standard == Standard::INTEROP) {
		ps.underline = optional_bool_attribute (node, "Underlined");
	} else {
		ps.underline = optional_bool_attribute (node, "Underline");
	}
	auto c = optional_string_attribute (node, "Color");
	if (c) {
		ps.colour = Colour (c.get ());
	}
	auto const e = optional_string_attribute (node, "Effect");
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
	auto hp = optional_number_attribute<float> (node, "HPosition");
	if (!hp) {
		hp = optional_number_attribute<float> (node, "Hposition");
	}
	if (hp) {
		ps.h_position = hp.get () / 100;
	}

	auto ha = optional_string_attribute (node, "HAlign");
	if (!ha) {
		ha = optional_string_attribute (node, "Halign");
	}
	if (ha) {
		ps.h_align = string_to_halign (ha.get ());
	}

	auto vp = optional_number_attribute<float> (node, "VPosition");
	if (!vp) {
		vp = optional_number_attribute<float> (node, "Vposition");
	}
	if (vp) {
		ps.v_position = vp.get () / 100;
	}

	auto va = optional_string_attribute (node, "VAlign");
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

	auto d = optional_string_attribute (node, "Direction");
	if (d) {
		ps.direction = string_to_direction (d.get ());
	}

	ps.type = ParseState::Type::TEXT;

	return ps;
}


SubtitleAsset::ParseState
SubtitleAsset::image_node_state (xmlpp::Element const * node) const
{
	ParseState ps;

	position_align (ps, node);

	ps.type = ParseState::Type::IMAGE;

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
	auto const u = optional_string_attribute(node, name).get_value_or ("");
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
SubtitleAsset::parse_subtitles (xmlpp::Element const * node, vector<ParseState>& state, optional<int> tcr, Standard standard)
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

	for (auto i: node->get_children()) {
		auto const v = dynamic_cast<xmlpp::ContentNode const *>(i);
		if (v) {
			maybe_add_subtitle (v->get_content(), state, standard);
		}
		auto const e = dynamic_cast<xmlpp::Element const *>(i);
		if (e) {
			parse_subtitles (e, state, tcr, standard);
		}
	}

	state.pop_back ();
}


void
SubtitleAsset::maybe_add_subtitle (string text, vector<ParseState> const & parse_state, Standard standard)
{
	if (empty_or_white_space (text)) {
		return;
	}

	ParseState ps;
	for (auto const& i: parse_state) {
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
	case ParseState::Type::TEXT:
		_subtitles.push_back (
			make_shared<SubtitleString>(
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
				ps.h_align.get_value_or(HAlign::CENTER),
				ps.v_position.get_value_or(0),
				ps.v_align.get_value_or(VAlign::CENTER),
				ps.direction.get_value_or (Direction::LTR),
				text,
				ps.effect.get_value_or (Effect::NONE),
				ps.effect_colour.get_value_or (dcp::Colour (0, 0, 0)),
				ps.fade_up_time.get_value_or(Time()),
				ps.fade_down_time.get_value_or(Time())
				)
			);
		break;
	case ParseState::Type::IMAGE:
	{
		switch (standard) {
		case Standard::INTEROP:
			if (text.size() >= 4) {
				/* Remove file extension */
				text = text.substr(0, text.size() - 4);
			}
			break;
		case Standard::SMPTE:
			/* It looks like this urn:uuid: is required, but DoM wasn't expecting it (and not writing it)
			 * until around 2.15.140 so I guess either:
			 *   a) it is not (always) used in the field, or
			 *   b) nobody noticed / complained.
			 */
			if (text.substr(0, 9) == "urn:uuid:") {
				text = text.substr(9);
			}
			break;
		}

		/* Add a subtitle with no image data and we'll fill that in later */
		_subtitles.push_back (
			make_shared<SubtitleImage>(
				ArrayData(),
				text,
				ps.in.get(),
				ps.out.get(),
				ps.h_position.get_value_or(0),
				ps.h_align.get_value_or(HAlign::CENTER),
				ps.v_position.get_value_or(0),
				ps.v_align.get_value_or(VAlign::CENTER),
				ps.fade_up_time.get_value_or(Time()),
				ps.fade_down_time.get_value_or(Time())
				)
			);
		break;
	}
	}
}


vector<shared_ptr<const Subtitle>>
SubtitleAsset::subtitles () const
{
	vector<shared_ptr<const Subtitle>> s;
	for (auto i: _subtitles) {
		s.push_back (i);
	}
	return s;
}


vector<shared_ptr<const Subtitle>>
SubtitleAsset::subtitles_during (Time from, Time to, bool starting) const
{
	vector<shared_ptr<const Subtitle>> s;
	for (auto i: _subtitles) {
		if ((starting && from <= i->in() && i->in() < to) || (!starting && i->out() >= from && i->in() <= to)) {
			s.push_back (i);
		}
	}

	return s;
}


/* XXX: this needs a test */
vector<shared_ptr<const Subtitle>>
SubtitleAsset::subtitles_in_reel (shared_ptr<const dcp::ReelAsset> asset) const
{
	auto frame_rate = asset->edit_rate().as_float();
	auto start = dcp::Time(asset->entry_point().get_value_or(0), frame_rate, time_code_rate());
	auto during = subtitles_during (start, start + dcp::Time(asset->intrinsic_duration(), frame_rate, time_code_rate()), false);

	vector<shared_ptr<const dcp::Subtitle>> corrected;
	for (auto i: during) {
		auto c = make_shared<dcp::Subtitle>(*i);
		c->set_in (c->in() - start);
		c->set_out (c->out() - start);
		corrected.push_back (c);
	}

	return corrected;
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
	for (auto i: _subtitles) {
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

	auto other = dynamic_pointer_cast<const SubtitleAsset> (other_asset);
	if (!other) {
		return false;
	}

	if (_subtitles.size() != other->_subtitles.size()) {
		note (NoteType::ERROR, String::compose("different number of subtitles: %1 vs %2", _subtitles.size(), other->_subtitles.size()));
		return false;
	}

	auto i = _subtitles.begin();
	auto j = other->_subtitles.begin();

	while (i != _subtitles.end()) {
		auto string_i = dynamic_pointer_cast<SubtitleString> (*i);
		auto string_j = dynamic_pointer_cast<SubtitleString> (*j);
		auto image_i = dynamic_pointer_cast<SubtitleImage> (*i);
		auto image_j = dynamic_pointer_cast<SubtitleImage> (*j);

		if ((string_i && !string_j) || (image_i && !image_j)) {
			note (NoteType::ERROR, "subtitles differ: string vs. image");
			return false;
		}

		if (string_i && *string_i != *string_j) {
			note (NoteType::ERROR, String::compose("subtitles differ in text or metadata: %1 vs %2", string_i->text(), string_j->text()));
			return false;
		}

		if (image_i && !image_i->equals(image_j, options, note)) {
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
	for (auto i: part->children) {
		pull_fonts (i);
	}

	if (part->parent) {
		/* Establish the common font features that each of part's children have;
		   these features go into part's font.
		*/
		part->font = part->children.front()->font;
		for (auto i: part->children) {
			part->font.take_intersection (i->font);
		}

		/* Remove common values from part's children's fonts */
		for (auto i: part->children) {
			i->font.take_difference (part->font);
		}
	}

	/* Merge adjacent children with the same font */
	auto i = part->children.begin();
	vector<shared_ptr<order::Part>> merged;

	while (i != part->children.end()) {

		if ((*i)->font.empty ()) {
			merged.push_back (*i);
			++i;
		} else {
			auto j = i;
			++j;
			while (j != part->children.end() && (*i)->font == (*j)->font) {
				++j;
			}
			if (std::distance (i, j) == 1) {
				merged.push_back (*i);
				++i;
			} else {
				shared_ptr<order::Part> group (new order::Part (part, (*i)->font));
				for (auto k = i; k != j; ++k) {
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
	auto sorted = _subtitles;
	std::stable_sort(sorted.begin(), sorted.end(), SubtitleSorter());

	/* Gather our subtitles into a hierarchy of Subtitle/Text/String objects, writing
	   font information into the bottom level (String) objects.
	*/

	auto root = make_shared<order::Part>(shared_ptr<order::Part>());
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

	for (auto i: sorted) {
		if (!subtitle ||
		    (last_in != i->in() ||
		     last_out != i->out() ||
		     last_fade_up_time != i->fade_up_time() ||
		     last_fade_down_time != i->fade_down_time())
			) {

			subtitle = make_shared<order::Subtitle>(root, i->in(), i->out(), i->fade_up_time(), i->fade_down_time());
			root->children.push_back (subtitle);

			last_in = i->in ();
			last_out = i->out ();
			last_fade_up_time = i->fade_up_time ();
			last_fade_down_time = i->fade_down_time ();
			text.reset ();
		}

		auto is = dynamic_pointer_cast<SubtitleString>(i);
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

			text->children.push_back (make_shared<order::String>(text, order::Font (is, standard), is->text()));
		}

		auto ii = dynamic_pointer_cast<SubtitleImage>(i);
		if (ii) {
			text.reset ();
			subtitle->children.push_back (
				make_shared<order::Image>(subtitle, ii->id(), ii->png_image(), ii->h_align(), ii->h_position(), ii->v_align(), ii->v_position())
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


map<string, ArrayData>
SubtitleAsset::font_data () const
{
	map<string, ArrayData> out;
	for (auto const& i: _fonts) {
		out[i.load_id] = i.data;
	}
	return out;
}


map<string, boost::filesystem::path>
SubtitleAsset::font_filenames () const
{
	map<string, boost::filesystem::path> out;
	for (auto const& i: _fonts) {
		if (i.file) {
			out[i.load_id] = *i.file;
		}
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
	vector<string> ids;
	for (auto i: load_font_nodes()) {
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

	for (auto i: load_font_nodes()) {
		if (i->id == "") {
			i->id = empty_id;
		}
	}

	for (auto i: _subtitles) {
		auto j = dynamic_pointer_cast<SubtitleString> (i);
		if (j && j->font() && j->font().get() == "") {
			j->set_font (empty_id);
		}
	}
}
