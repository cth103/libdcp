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


/** @file  src/subtitle_string.cc
 *  @brief SubtitleString class
 */


#include "subtitle_string.h"
#include "xml.h"
#include <cmath>


using std::max;
using std::min;
using std::ostream;
using std::string;
using boost::optional;
using namespace dcp;


SubtitleString::SubtitleString (
	optional<string> font,
	bool italic,
	bool bold,
	bool underline,
	Colour colour,
	int size,
	float aspect_adjust,
	Time in,
	Time out,
	float h_position,
	HAlign h_align,
	float v_position,
	VAlign v_align,
	Direction direction,
	string text,
	Effect effect,
	Colour effect_colour,
	Time fade_up_time,
	Time fade_down_time
	)
	: Subtitle (in, out, h_position, h_align, v_position, v_align, fade_up_time, fade_down_time)
	, _font (font)
	, _italic (italic)
	, _bold (bold)
	, _underline (underline)
	, _colour (colour)
	, _size (size)
	, _aspect_adjust (aspect_adjust)
	, _direction (direction)
	, _text (text)
	, _effect (effect)
	, _effect_colour (effect_colour)
{
	_aspect_adjust = max(min(_aspect_adjust, 4.0f), 0.25f);
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
		a.bold() == b.bold() &&
		a.underline() == b.underline() &&
		a.colour() == b.colour() &&
		a.size() == b.size() &&
		fabs (a.aspect_adjust() - b.aspect_adjust()) < ASPECT_ADJUST_EPSILON &&
		a.in() == b.in() &&
		a.out() == b.out() &&
		a.h_position() == b.h_position() &&
		a.h_align() == b.h_align() &&
		a.v_position() == b.v_position() &&
		a.v_align() == b.v_align() &&
		a.direction() == b.direction() &&
		a.text() == b.text() &&
		a.effect() == b.effect() &&
		a.effect_colour() == b.effect_colour() &&
		a.fade_up_time() == b.fade_up_time() &&
		a.fade_down_time() == b.fade_down_time()
		);
}


bool
dcp::operator!= (SubtitleString const & a, SubtitleString const & b)
{
	return !(a == b);
}


ostream&
dcp::operator<< (ostream& s, SubtitleString const & sub)
{
	s << "\n`" << sub.text() << "' from " << sub.in() << " to " << sub.out() << ";\n"
	  << "fade up " << sub.fade_up_time() << ", fade down " << sub.fade_down_time() << ";\n"
	  << "font " << sub.font().get_value_or ("[default]") << ", ";

	if (sub.italic()) {
		s << "italic, ";
	} else {
		s << "non-italic, ";
	}

	if (sub.bold()) {
		s << "bold, ";
	} else {
		s << "normal, ";
	}

	if (sub.underline()) {
		s << "underlined, ";
	}

	s << "size " << sub.size() << ", aspect " << sub.aspect_adjust()
	  << ", colour (" << sub.colour().r << ", " << sub.colour().g << ", " << sub.colour().b << ")"
	  << ", vpos " << sub.v_position() << ", valign " << ((int) sub.v_align())
	  << ", hpos " << sub.h_position() << ", halign " << ((int) sub.h_align())
	  << ", direction " << ((int) sub.direction())
	  << ", effect " << ((int) sub.effect())
	  << ", effect colour (" << sub.effect_colour().r << ", " << sub.effect_colour().g << ", " << sub.effect_colour().b << ")";

	return s;
}
