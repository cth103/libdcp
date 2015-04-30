/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#include "subtitle_string.h"
#include "xml.h"

using std::string;
using std::ostream;
using boost::optional;
using namespace dcp;

/** @param v_position Vertical position as a fraction of the screen height (between 0 and 1) from v_align */
SubtitleString::SubtitleString (
	optional<string> font,
	bool italic,
	Colour colour,
	int size,
	Time in,
	Time out,
	float v_position,
	VAlign v_align,
	string text,
	Effect effect,
	Colour effect_colour,
	Time fade_up_time,
	Time fade_down_time
	)
	: _font (font)
	, _italic (italic)
	, _colour (colour)
	, _size (size)
	, _in (in)
	, _out (out)
	, _v_position (v_position)
	, _v_align (v_align)
	, _text (text)
	, _effect (effect)
	, _effect_colour (effect_colour)
	, _fade_up_time (fade_up_time)
	, _fade_down_time (fade_down_time)
{

}

int
SubtitleString::size_in_pixels (int screen_height) const
{
	/* Size in the subtitle file is given in points as if the screen
	   height is 11 inches, so a 72pt font would be 1/11th of the screen
	   height.
	*/
	
	return _size * screen_height / (11 * 72);
}

bool
dcp::operator== (SubtitleString const & a, SubtitleString const & b)
{
	return (
		a.font() == b.font() &&
		a.italic() == b.italic() &&
		a.colour() == b.colour() &&
		a.size() == b.size() &&
		a.in() == b.in() &&
		a.out() == b.out() &&
		a.v_position() == b.v_position() &&
		a.v_align() == b.v_align() &&
		a.text() == b.text() &&
		a.effect() == b.effect() &&
		a.effect_colour() == b.effect_colour() &&
		a.fade_up_time() == b.fade_up_time() &&
		a.fade_down_time() == b.fade_down_time()
		);
}

ostream&
dcp::operator<< (ostream& s, SubtitleString const & sub)
{
	s << "\n`" << sub.text() << "' from " << sub.in() << " to " << sub.out() << ";\n"
	  << "fade up " << sub.fade_up_time() << ", fade down " << sub.fade_down_time() << ";\n"
	  << "font " << sub.font().get_value_or ("[default]") << ", ";

	if (sub.italic()) {
		s << "italic";
	} else {
		s << "non-italic";
	}
	
	s << ", size " << sub.size() << ", colour " << sub.colour() << ", vpos " << sub.v_position() << ", valign " << ((int) sub.v_align()) << ";\n"
	  << "effect " << ((int) sub.effect()) << ", effect colour " << sub.effect_colour();

	return s;
}
