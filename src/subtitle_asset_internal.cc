/*
    Copyright (C) 2012-2018 Carl Hetherington <cth@carlh.net>

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

#include "subtitle_asset_internal.h"
#include "subtitle_string.h"
#include <cmath>

using std::string;
using std::map;
using namespace dcp;

string
order::Context::xmlns () const
{
	return standard == SMPTE ? "dcst" : "";
}

order::Font::Font (SubtitleString const & s, Standard standard)
{
	if (s.font()) {
		if (standard == SMPTE) {
			_values["ID"] = s.font().get ();
		} else {
			_values["Id"] = s.font().get ();
		}
	}
	_values["Italic"] = s.italic() ? "yes" : "no";
	_values["Color"] = s.colour().to_argb_string();
	_values["Size"] = raw_convert<string> (s.size());
	_values["AspectAdjust"] = raw_convert<string>(s.aspect_adjust(), 1, true);
	_values["Effect"] = effect_to_string (s.effect());
	_values["EffectColor"] = s.effect_colour().to_argb_string();
	_values["Script"] = "normal";
	if (standard == SMPTE) {
		_values["Underline"] = s.underline() ? "yes" : "no";
	} else {
		_values["Underlined"] = s.underline() ? "yes" : "no";
	}
	_values["Weight"] = s.bold() ? "bold" : "normal";
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

	for (map<string, string>::const_iterator i = other._values.begin(); i != other._values.end(); ++i) {
		map<string, string>::iterator t = _values.find (i->first);
		if (t != _values.end() && t->second == i->second) {
			inter.insert (*i);
		}
	}

	_values = inter;
}

/** Modify our values so that it contains only those keys that are not in other */
void
order::Font::take_difference (Font other)
{
	map<string, string> diff;
	for (map<string, string>::const_iterator i = _values.begin(); i != _values.end(); ++i) {
		if (other._values.find (i->first) == other._values.end ()) {
			diff.insert (*i);
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

	BOOST_FOREACH (boost::shared_ptr<order::Part> i, children) {
		i->write_xml (parent, context);
	}
}

xmlpp::Element*
order::Text::as_xml (xmlpp::Element* parent, Context& context) const
{
	xmlpp::Element* e = parent->add_child ("Text", context.xmlns());

	if (_h_align != HALIGN_CENTER) {
		if (context.standard == SMPTE) {
			e->set_attribute ("Halign", halign_to_string (_h_align));
		} else {
			e->set_attribute ("HAlign", halign_to_string (_h_align));
		}
	}

	if (fabs(_h_position) > ALIGN_EPSILON) {
		if (context.standard == SMPTE) {
			e->set_attribute ("Hposition", raw_convert<string> (_h_position * 100, 6));
		} else {
			e->set_attribute ("HPosition", raw_convert<string> (_h_position * 100, 6));
		}
	}

	if (context.standard == SMPTE) {
		e->set_attribute ("Valign", valign_to_string (_v_align));
	} else {
		e->set_attribute ("VAlign", valign_to_string (_v_align));
	}

	if (fabs(_v_position) > ALIGN_EPSILON) {
		if (context.standard == SMPTE) {
			e->set_attribute ("Vposition", raw_convert<string> (_v_position * 100, 6));
		} else {
			e->set_attribute ("VPosition", raw_convert<string> (_v_position * 100, 6));
		}
	} else {
		if (context.standard == SMPTE) {
			e->set_attribute ("Vposition", "0");
		} else {
			e->set_attribute ("VPosition", "0");
		}
	}

	/* Interop only supports "horizontal" or "vertical" for direction, so only write this
	   for SMPTE.
	*/
	if (_direction != DIRECTION_LTR && context.standard == SMPTE) {
		e->set_attribute ("Direction", direction_to_string (_direction));
	}

	return e;
}

xmlpp::Element*
order::Subtitle::as_xml (xmlpp::Element* parent, Context& context) const
{
	xmlpp::Element* e = parent->add_child ("Subtitle", context.xmlns());
	e->set_attribute ("SpotNumber", raw_convert<string> (context.spot_number++));
	e->set_attribute ("TimeIn", _in.rebase(context.time_code_rate).as_string(context.standard));
	e->set_attribute ("TimeOut", _out.rebase(context.time_code_rate).as_string(context.standard));
	if (context.standard == SMPTE) {
		e->set_attribute ("FadeUpTime", _fade_up.rebase(context.time_code_rate).as_string(context.standard));
		e->set_attribute ("FadeDownTime", _fade_down.rebase(context.time_code_rate).as_string(context.standard));
	} else {
		e->set_attribute ("FadeUpTime", raw_convert<string> (_fade_up.as_editable_units(context.time_code_rate)));
		e->set_attribute ("FadeDownTime", raw_convert<string> (_fade_down.as_editable_units(context.time_code_rate)));
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
