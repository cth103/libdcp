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


/** @file  src/subtitle_asset_internal.cc
 *  @brief Internal SubtitleAsset helpers
 */


#include "subtitle_asset_internal.h"
#include "subtitle_string.h"
#include "compose.hpp"
#include <cmath>


using std::string;
using std::map;
using std::shared_ptr;
using namespace dcp;


string
order::Context::xmlns () const
{
	return standard == Standard::SMPTE ? "dcst" : "";
}


order::Font::Font (shared_ptr<SubtitleString> s, Standard standard)
{
	if (s->font()) {
		if (standard == Standard::SMPTE) {
			_values["ID"] = s->font().get ();
		} else {
			_values["Id"] = s->font().get ();
		}
	}
	_values["Italic"] = s->italic() ? "yes" : "no";
	_values["Color"] = s->colour().to_argb_string();
	_values["Size"] = raw_convert<string> (s->size());
	_values["AspectAdjust"] = raw_convert<string>(s->aspect_adjust(), 1, true);
	_values["Effect"] = effect_to_string (s->effect());
	_values["EffectColor"] = s->effect_colour().to_argb_string();
	_values["Script"] = "normal";
	if (standard == Standard::SMPTE) {
		_values["Underline"] = s->underline() ? "yes" : "no";
	} else {
		_values["Underlined"] = s->underline() ? "yes" : "no";
	}
	_values["Weight"] = s->bold() ? "bold" : "normal";
}


xmlpp::Element*
order::Font::as_xml (xmlpp::Element* parent, Context& context) const
{
	xmlpp::Element* e = parent->add_child ("Font", context.xmlns());
	for (map<string, string>::const_iterator i = _values.begin(); i != _values.end(); ++i) {
		e->set_attribute (i->first, i->second);
	}
	return e;
}


/** Modify our values so that they contain only those that are common to us and
 *  other.
 */
void
order::Font::take_intersection (Font other)
{
	map<string, string> inter;

	for (auto const& i: other._values) {
		auto t = _values.find (i.first);
		if (t != _values.end() && t->second == i.second) {
			inter.insert (i);
		}
	}

	_values = inter;
}


/** Modify our values so that it contains only those keys that are not in other */
void
order::Font::take_difference (Font other)
{
	map<string, string> diff;
	for (auto const& i: _values) {
		if (other._values.find (i.first) == other._values.end()) {
			diff.insert (i);
		}
	}

	_values = diff;
}


bool
order::Font::empty () const
{
	return _values.empty ();
}


xmlpp::Element*
order::Part::as_xml (xmlpp::Element* parent, Context &) const
{
	return parent;
}


xmlpp::Element*
order::String::as_xml (xmlpp::Element* parent, Context &) const
{
	parent->add_child_text (text);
	return 0;
}


void
order::Part::write_xml (xmlpp::Element* parent, order::Context& context) const
{
	if (!font.empty ()) {
		parent = font.as_xml (parent, context);
	}

	parent = as_xml (parent, context);

	for (auto i: children) {
		i->write_xml (parent, context);
	}
}


static void
position_align (xmlpp::Element* e, order::Context& context, HAlign h_align, float h_position, VAlign v_align, float v_position)
{
	if (h_align != HAlign::CENTER) {
		if (context.standard == Standard::SMPTE) {
			e->set_attribute ("Halign", halign_to_string (h_align));
		} else {
			e->set_attribute ("HAlign", halign_to_string (h_align));
		}
	}

	if (fabs(h_position) > ALIGN_EPSILON) {
		if (context.standard == Standard::SMPTE) {
			e->set_attribute ("Hposition", raw_convert<string> (h_position * 100, 6));
		} else {
			e->set_attribute ("HPosition", raw_convert<string> (h_position * 100, 6));
		}
	}

	if (context.standard == Standard::SMPTE) {
		e->set_attribute ("Valign", valign_to_string (v_align));
	} else {
		e->set_attribute ("VAlign", valign_to_string (v_align));
	}

	if (fabs(v_position) > ALIGN_EPSILON) {
		if (context.standard == Standard::SMPTE) {
			e->set_attribute ("Vposition", raw_convert<string> (v_position * 100, 6));
		} else {
			e->set_attribute ("VPosition", raw_convert<string> (v_position * 100, 6));
		}
	} else {
		if (context.standard == Standard::SMPTE) {
			e->set_attribute ("Vposition", "0");
		} else {
			e->set_attribute ("VPosition", "0");
		}
	}
}


xmlpp::Element*
order::Text::as_xml (xmlpp::Element* parent, Context& context) const
{
	auto e = parent->add_child ("Text", context.xmlns());

	position_align (e, context, _h_align, _h_position, _v_align, _v_position);

	/* Interop only supports "horizontal" or "vertical" for direction, so only write this
	   for SMPTE.
	*/
	if (_direction != Direction::LTR && context.standard == Standard::SMPTE) {
		e->set_attribute ("Direction", direction_to_string (_direction));
	}

	return e;
}


xmlpp::Element*
order::Subtitle::as_xml (xmlpp::Element* parent, Context& context) const
{
	auto e = parent->add_child ("Subtitle", context.xmlns());
	e->set_attribute ("SpotNumber", raw_convert<string> (context.spot_number++));
	e->set_attribute ("TimeIn", _in.rebase(context.time_code_rate).as_string(context.standard));
	e->set_attribute ("TimeOut", _out.rebase(context.time_code_rate).as_string(context.standard));
	if (context.standard == Standard::SMPTE) {
		e->set_attribute ("FadeUpTime", _fade_up.rebase(context.time_code_rate).as_string(context.standard));
		e->set_attribute ("FadeDownTime", _fade_down.rebase(context.time_code_rate).as_string(context.standard));
	} else {
		e->set_attribute ("FadeUpTime", raw_convert<string> (_fade_up.as_editable_units_ceil(context.time_code_rate)));
		e->set_attribute ("FadeDownTime", raw_convert<string> (_fade_down.as_editable_units_ceil(context.time_code_rate)));
	}
	return e;
}


bool
order::Font::operator== (Font const & other) const
{
	return _values == other._values;
}


void
order::Font::clear ()
{
	_values.clear ();
}


xmlpp::Element *
order::Image::as_xml (xmlpp::Element* parent, Context& context) const
{
	auto e = parent->add_child ("Image", context.xmlns());

	position_align (e, context, _h_align, _h_position, _v_align, _v_position);
	if (context.standard == Standard::SMPTE) {
		e->add_child_text ("urn:uuid:" + _id);
	} else {
		e->add_child_text (_id + ".png");
	}

	return e;
}
