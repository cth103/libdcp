/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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
#include "subtitle_asset.h"
#include "util.h"
#include "xml.h"
#include "font_node.h"
#include "text_node.h"
#include "subtitle_string.h"
#include "dcp_assert.h"
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_util.h>
#include <libxml++/nodes/element.h>
#include <boost/algorithm/string.hpp>
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
using namespace dcp;

SubtitleAsset::SubtitleAsset ()
{

}

SubtitleAsset::SubtitleAsset (boost::filesystem::path file)
	: Asset (file)
{

}

void
SubtitleAsset::parse_subtitles (
	shared_ptr<cxml::Document> xml,
	list<shared_ptr<dcp::FontNode> > font_nodes,
	list<shared_ptr<dcp::SubtitleNode> > subtitle_nodes
	)
{
	/* Make Subtitle objects to represent the raw XML nodes in a sane way */
	ParseState parse_state;
	examine_nodes (xml, font_nodes, parse_state);
	examine_nodes (xml, subtitle_nodes, parse_state);
}

void
SubtitleAsset::examine_nodes (
	shared_ptr<const cxml::Node> xml,
	list<shared_ptr<dcp::SubtitleNode> > const & subtitle_nodes,
	ParseState& parse_state
	)
{
	BOOST_FOREACH (shared_ptr<dcp::SubtitleNode> i, subtitle_nodes) {
		parse_state.subtitle_nodes.push_back (i);
		examine_nodes (xml, i->text_nodes, parse_state);
		examine_nodes (xml, i->font_nodes, parse_state);
		parse_state.subtitle_nodes.pop_back ();
	}
}

void
SubtitleAsset::examine_nodes (
	shared_ptr<const cxml::Node> xml,
	list<shared_ptr<dcp::FontNode> > const & font_nodes,
	ParseState& parse_state
	)
{
	BOOST_FOREACH (shared_ptr<dcp::FontNode> i, font_nodes) {

		parse_state.font_nodes.push_back (i);
		maybe_add_subtitle (i->text, parse_state);

		examine_nodes (xml, i->subtitle_nodes, parse_state);
		examine_nodes (xml, i->font_nodes, parse_state);
		examine_nodes (xml, i->text_nodes, parse_state);

		parse_state.font_nodes.pop_back ();
	}
}

void
SubtitleAsset::examine_nodes (
	shared_ptr<const cxml::Node> xml,
	list<shared_ptr<dcp::TextNode> > const & text_nodes,
	ParseState& parse_state
	)
{
	BOOST_FOREACH (shared_ptr<dcp::TextNode> i, text_nodes) {
		parse_state.text_nodes.push_back (i);
		maybe_add_subtitle (i->text, parse_state);
		examine_nodes (xml, i->font_nodes, parse_state);
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

	DCP_ASSERT (!parse_state.text_nodes.empty ());
	DCP_ASSERT (!parse_state.subtitle_nodes.empty ());

	dcp::FontNode effective_font (parse_state.font_nodes);
	dcp::TextNode effective_text (*parse_state.text_nodes.back ());
	dcp::SubtitleNode effective_subtitle (*parse_state.subtitle_nodes.back ());

	_subtitles.push_back (
		SubtitleString (
			effective_font.id,
			effective_font.italic.get_value_or (false),
			effective_font.bold.get_value_or (false),
			effective_font.underline.get_value_or (false),
			effective_font.colour.get_value_or (dcp::Colour (255, 255, 255)),
			effective_font.size,
			effective_font.aspect_adjust.get_value_or (1.0),
			effective_subtitle.in,
			effective_subtitle.out,
			effective_text.h_position,
			effective_text.h_align,
			effective_text.v_position,
			effective_text.v_align,
			effective_text.direction,
			text,
			effective_font.effect.get_value_or (NONE),
			effective_font.effect_colour.get_value_or (dcp::Colour (0, 0, 0)),
			effective_subtitle.fade_up_time,
			effective_subtitle.fade_down_time
			)
		);
}

list<SubtitleString>
SubtitleAsset::subtitles_during (Time from, Time to, bool starting) const
{
	list<SubtitleString> s;
	BOOST_FOREACH (SubtitleString const & i, _subtitles) {
		if ((starting && from <= i.in() && i.in() < to) || (!starting && i.out() >= from && i.in() <= to)) {
			s.push_back (i);
		}
	}

	return s;
}

void
SubtitleAsset::add (SubtitleString s)
{
	_subtitles.push_back (s);
}

Time
SubtitleAsset::latest_subtitle_out () const
{
	Time t;
	BOOST_FOREACH (SubtitleString const & i, _subtitles) {
		if (i.out() > t) {
			t = i.out ();
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

	if (_subtitles != other->_subtitles) {
		note (DCP_ERROR, "subtitles differ");
		return false;
	}

	return true;
}

struct SubtitleSorter {
	bool operator() (SubtitleString const & a, SubtitleString const & b) {
		if (a.in() != b.in()) {
			return a.in() < b.in();
		}
		return a.v_position() < b.v_position();
	}
};

/** @param standard Standard (INTEROP or SMPTE); this is used rather than putting things in the child
 *  class because the differences between the two are fairly subtle.
 */
void
SubtitleAsset::subtitles_as_xml (xmlpp::Element* root, int time_code_rate, Standard standard) const
{
	list<SubtitleString> sorted = _subtitles;
	sorted.sort (SubtitleSorter ());

	string const xmlns = standard == SMPTE ? "dcst" : "";

	/* XXX: script not supported */

	optional<string> font;
	bool italic = false;
	bool bold = false;
	bool underline = false;
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

	BOOST_FOREACH (SubtitleString const & i, sorted) {

		/* We will start a new <Font>...</Font> whenever some font property changes.
		   I suppose we should really make an optimal hierarchy of <Font> tags, but
		   that seems hard.
		*/

		bool const font_changed =
			font          != i.font()          ||
			italic        != i.italic()        ||
			bold          != i.bold()          ||
			underline     != i.underline()     ||
			colour        != i.colour()        ||
			size          != i.size()          ||
			fabs (aspect_adjust - i.aspect_adjust()) > ASPECT_ADJUST_EPSILON ||
			effect        != i.effect()        ||
			effect_colour != i.effect_colour();

		if (font_changed) {
			font = i.font ();
			italic = i.italic ();
			bold = i.bold ();
			underline = i.underline ();
			colour = i.colour ();
			size = i.size ();
			aspect_adjust = i.aspect_adjust ();
			effect = i.effect ();
			effect_colour = i.effect_colour ();
		}

		if (!font_element || font_changed) {
			font_element = root->add_child ("Font", xmlns);
			if (font) {
				if (standard == SMPTE) {
					font_element->set_attribute ("ID", font.get ());
				} else {
					font_element->set_attribute ("Id", font.get ());
				}
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
			if (standard == SMPTE) {
				font_element->set_attribute ("Underline", underline ? "yes" : "no");
			} else {
				font_element->set_attribute ("Underlined", underline ? "yes" : "no");
			}
			font_element->set_attribute ("Weight", bold ? "bold" : "normal");
		}

		if (!subtitle_element || font_changed ||
		    (last_in != i.in() ||
		     last_out != i.out() ||
		     last_fade_up_time != i.fade_up_time() ||
		     last_fade_down_time != i.fade_down_time()
			    )) {

			subtitle_element = font_element->add_child ("Subtitle", xmlns);
			subtitle_element->set_attribute ("SpotNumber", raw_convert<string> (spot_number++));
			subtitle_element->set_attribute ("TimeIn", i.in().rebase(time_code_rate).as_string(standard));
			subtitle_element->set_attribute ("TimeOut", i.out().rebase(time_code_rate).as_string(standard));
			if (standard == SMPTE) {
				subtitle_element->set_attribute ("FadeUpTime", i.fade_up_time().rebase(time_code_rate).as_string(standard));
				subtitle_element->set_attribute ("FadeDownTime", i.fade_down_time().rebase(time_code_rate).as_string(standard));
			} else {
				subtitle_element->set_attribute ("FadeUpTime", raw_convert<string> (i.fade_up_time().as_editable_units(time_code_rate)));
				subtitle_element->set_attribute ("FadeDownTime", raw_convert<string> (i.fade_down_time().as_editable_units(time_code_rate)));
			}

			last_in = i.in ();
			last_out = i.out ();
			last_fade_up_time = i.fade_up_time ();
			last_fade_down_time = i.fade_down_time ();
		}

		xmlpp::Element* text = subtitle_element->add_child ("Text", xmlns);

		if (i.h_align() != HALIGN_CENTER) {
			if (standard == SMPTE) {
				text->set_attribute ("Halign", halign_to_string (i.h_align ()));
			} else {
				text->set_attribute ("HAlign", halign_to_string (i.h_align ()));
			}
		}

		if (i.h_position() > ALIGN_EPSILON) {
			if (standard == SMPTE) {
				text->set_attribute ("Hposition", raw_convert<string> (i.h_position() * 100, 6));
			} else {
				text->set_attribute ("HPosition", raw_convert<string> (i.h_position() * 100, 6));
			}
		}

		if (standard == SMPTE) {
			text->set_attribute ("Valign", valign_to_string (i.v_align()));
		} else {
			text->set_attribute ("VAlign", valign_to_string (i.v_align()));
		}

		if (i.v_position() > ALIGN_EPSILON) {
			if (standard == SMPTE) {
				text->set_attribute ("Vposition", raw_convert<string> (i.v_position() * 100, 6));
			} else {
				text->set_attribute ("VPosition", raw_convert<string> (i.v_position() * 100, 6));
			}
		} else {
			if (standard == SMPTE) {
				text->set_attribute ("Vposition", "0");
			} else {
				text->set_attribute ("VPosition", "0");
			}
		}

		/* Interop only supports "horizontal" or "vertical" for direction, so only write this
		   for SMPTE.
		*/
		if (i.direction() != DIRECTION_LTR && standard == SMPTE) {
			text->set_attribute ("Direction", direction_to_string (i.direction ()));
		}

		text->add_child_text (i.text());
	}
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
