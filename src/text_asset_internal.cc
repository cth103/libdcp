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


/** @file  src/text_asset_internal.cc
 *  @brief Internal TextAsset helpers
 */


#include "text_asset_internal.h"
#include "text_string.h"
#include "compose.hpp"
#include <fmt/format.h>
#include <cmath>


using std::dynamic_pointer_cast;
using std::string;
using std::map;
using std::shared_ptr;
using std::vector;
using boost::optional;
using namespace dcp;


order::Font::Font(shared_ptr<TextString> s, Standard standard)
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
	_values["Size"] = fmt::to_string(s->size());
	_values["AspectAdjust"] = fmt::format("{:.1f}", s->aspect_adjust());
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
order::Font::as_xml (xmlpp::Element* parent, Context&) const
{
	auto e = cxml::add_child(parent, "Font");
	for (const auto& i: _values) {
		e->set_attribute (i.first, i.second);
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
order::String::as_xml (xmlpp::Element* parent, Context& context) const
{
	if (fabs(_space_before) > SPACE_BEFORE_EPSILON) {
		auto space = cxml::add_child(parent, "Space");
		auto size = fmt::format("{:.2}", _space_before);
		if (context.standard == Standard::INTEROP) {
			size += "em";
		}
		space->set_attribute("Size", size);
	}
	parent->add_child_text (_text);
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
position_align(
		xmlpp::Element* e,
		order::Context& context,
		HAlign h_align,
		float h_position,
		VAlign v_align,
		float v_position,
		float z_position,
		optional<string> variable_z
		)
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
			e->set_attribute("Hposition", fmt::format("{:.6}", h_position * 100));
		} else {
			e->set_attribute("HPosition", fmt::format("{:.6}", h_position * 100));
		}
	}

	if (context.standard == Standard::SMPTE) {
		e->set_attribute ("Valign", valign_to_string (v_align));
	} else {
		e->set_attribute ("VAlign", valign_to_string (v_align));
	}

	if (fabs(v_position) > ALIGN_EPSILON) {
		if (context.standard == Standard::SMPTE) {
			e->set_attribute("Vposition", fmt::format("{:.6}", v_position * 100));
		} else {
			e->set_attribute("VPosition", fmt::format("{:.6}", v_position * 100));
		}
	} else {
		if (context.standard == Standard::SMPTE) {
			e->set_attribute ("Vposition", "0");
		} else {
			e->set_attribute ("VPosition", "0");
		}
	}

	if (fabs(z_position) > ALIGN_EPSILON && context.standard == Standard::SMPTE) {
		e->set_attribute("Zposition", fmt::format("{:.6}", z_position * 100));
	}

	if (variable_z) {
		e->set_attribute("VariableZ", *variable_z);
	}
}


xmlpp::Element*
order::Text::as_xml (xmlpp::Element* parent, Context& context) const
{
	auto e = cxml::add_child(parent, "Text");

	position_align(e, context, _h_align, _h_position, _v_align, _v_position, _z_position, _variable_z);

	/* Interop only supports "horizontal" or "vertical" for direction, so only write this
	   for SMPTE.
	*/
	if (_direction != Direction::LTR && context.standard == Standard::SMPTE) {
		e->set_attribute ("Direction", direction_to_string (_direction));
	}

	for (auto const& ruby: _rubies) {
		auto xml = cxml::add_child(e, "Ruby");
		cxml::add_child(xml, "Rb")->add_child_text(ruby.base);
		auto rt = cxml::add_child(xml, "Rt");
		rt->add_child_text(ruby.annotation);
		rt->set_attribute("Size", fmt::format("{:.6}", ruby.size));
		rt->set_attribute("Position", ruby.position == RubyPosition::BEFORE ? "before" : "after");
		rt->set_attribute("Offset", fmt::format("{:.6}", ruby.offset));
		rt->set_attribute("Spacing", fmt::format("{:.6}", ruby.spacing));
		rt->set_attribute("AspectAdjust", fmt::format("{:.6}", ruby.aspect_adjust));
	}

	return e;
}


optional<string>
order::Subtitle::find_or_add_variable_z_positions(vector<dcp::Text::VariableZPosition> const& positions, int& load_variable_z_index)
{
	if (positions.empty()) {
		return {};
	}

	auto iter = std::find_if(_load_variable_z.begin(), _load_variable_z.end(), [positions](LoadVariableZ const& load) { return positions == load.positions(); });
	if (iter == _load_variable_z.end()) {
		auto const id = fmt::format("Zvector{}", load_variable_z_index++);
		_load_variable_z.push_back(LoadVariableZ(id, positions));
		return id;
	}

	return iter->id();
}


xmlpp::Element*
order::Subtitle::as_xml (xmlpp::Element* parent, Context& context) const
{
	auto e = cxml::add_child(parent, "Subtitle");
	e->set_attribute("SpotNumber", fmt::to_string(context.spot_number++));
	e->set_attribute ("TimeIn", _in.rebase(context.time_code_rate).as_string(context.standard));
	e->set_attribute ("TimeOut", _out.rebase(context.time_code_rate).as_string(context.standard));
	if (context.standard == Standard::SMPTE) {
		e->set_attribute ("FadeUpTime", _fade_up.rebase(context.time_code_rate).as_string(context.standard));
		e->set_attribute ("FadeDownTime", _fade_down.rebase(context.time_code_rate).as_string(context.standard));
	} else {
		e->set_attribute("FadeUpTime", fmt::to_string(_fade_up.as_editable_units_ceil(context.time_code_rate)));
		e->set_attribute("FadeDownTime", fmt::to_string(_fade_down.as_editable_units_ceil(context.time_code_rate)));
	}

	for (auto const& vz: _load_variable_z) {
		vz.as_xml(cxml::add_child(e, "LoadVariableZ"));
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
	auto e = cxml::add_child(parent, "Image");

	position_align(e, context, _h_align, _h_position, _v_align, _v_position, _z_position, _variable_z);
	if (context.standard == Standard::SMPTE) {
		e->add_child_text ("urn:uuid:" + _id);
	} else {
		e->add_child_text (_id + ".png");
	}

	return e;
}

