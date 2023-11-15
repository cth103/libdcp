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


#include "compose.hpp"
#include "subtitle_string.h"
#include "xml.h"
#include <cmath>


using std::dynamic_pointer_cast;
using std::max;
using std::min;
using std::ostream;
using std::shared_ptr;
using std::string;
using std::vector;
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
	float z_position,
	Direction direction,
	string text,
	Effect effect,
	Colour effect_colour,
	Time fade_up_time,
	Time fade_down_time,
	float space_before,
	vector<Ruby> rubies
	)
	: Subtitle(in, out, h_position, h_align, v_position, v_align, z_position, fade_up_time, fade_down_time)
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
	, _space_before (space_before)
	, _rubies(rubies)
{
	_aspect_adjust = max(min(_aspect_adjust, 4.0f), 0.25f);
}


float
SubtitleString::size_in_pixels (int screen_height) const
{
	/* Size in the subtitle file is given in points as if the screen
	   height is 11 inches, so a 72pt font would be 1/11th of the screen
	   height.
	*/

	return _size * static_cast<float>(screen_height) / (11.0f * 72.0f);
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
		a.z_position() == b.z_position() &&
		a.direction() == b.direction() &&
		a.text() == b.text() &&
		a.effect() == b.effect() &&
		a.effect_colour() == b.effect_colour() &&
		a.fade_up_time() == b.fade_up_time() &&
		a.fade_down_time() == b.fade_down_time() &&
		fabs (a.space_before() - b.space_before()) < SPACE_BEFORE_EPSILON &&
		a.rubies() == b.rubies()
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
	  << ", zpos " << sub.z_position()
	  << ", direction " << ((int) sub.direction())
	  << ", effect " << ((int) sub.effect())
	  << ", effect colour (" << sub.effect_colour().r << ", " << sub.effect_colour().g << ", " << sub.effect_colour().b << ")"
	  << ", space before " << sub.space_before();

	for (auto ruby: sub.rubies()) {
		s << ", ruby " << ruby.base << " " << ruby.annotation;
	}

	return s;
}


bool
SubtitleString::equals(shared_ptr<const Subtitle> other_sub, EqualityOptions const& options, NoteHandler note) const
{
	if (!Subtitle::equals(other_sub, options, note)) {
		return false;
	}

	auto other = dynamic_pointer_cast<const SubtitleString>(other_sub);
	if (!other) {
		note(NoteType::ERROR, "Subtitle types differ: string vs image");
		return false;
	}

	bool same = true;

	if (_font != other->_font) {
		note(NoteType::ERROR, String::compose("subtitle font differs: %1 vs %2", _font.get_value_or("[none]"), other->_font.get_value_or("[none]")));
		same = false;
	}

	if (_italic != other->_italic) {
		note(NoteType::ERROR, String::compose("subtitle italic flag differs: %1 vs %2", _italic ? "true" : "false", other->_italic ? "true" : "false"));
		same = false;
	}

	if (_bold != other->_bold) {
		note(NoteType::ERROR, String::compose("subtitle bold flag differs: %1 vs %2", _bold ? "true" : "false", other->_bold ? "true" : "false"));
		same = false;
	}

	if (_underline != other->_underline) {
		note(NoteType::ERROR, String::compose("subtitle underline flag differs: %1 vs %2", _underline ? "true" : "false", other->_underline ? "true" : "false"));
		same = false;
	}

	if (_colour != other->_colour) {
		note(NoteType::ERROR, String::compose("subtitle colour differs: %1 vs %2", _colour.to_rgb_string(), other->_colour.to_rgb_string()));
		same = false;
	}

	if (_size != other->_size) {
		note(NoteType::ERROR, String::compose("subtitle size differs: %1 vs %2", _size, other->_size));
		same = false;
	}

	if (_aspect_adjust != other->_aspect_adjust) {
		note(NoteType::ERROR, String::compose("subtitle aspect_adjust differs: %1 vs %2", _aspect_adjust, other->_aspect_adjust));
		same = false;
	}

	if (_direction != other->_direction) {
		note(NoteType::ERROR, String::compose("subtitle direction differs: %1 vs %2", direction_to_string(_direction), direction_to_string(other->_direction)));
		same = false;
	}

	if (_text != other->_text) {
		note(NoteType::ERROR, String::compose("subtitle text differs: %1 vs %2", _text, other->_text));
		same = false;
	}

	if (_effect != other->_effect) {
		note(NoteType::ERROR, String::compose("subtitle effect differs: %1 vs %2", effect_to_string(_effect), effect_to_string(other->_effect)));
		same = false;
	}

	if (_effect_colour != other->_effect_colour) {
		note(NoteType::ERROR, String::compose("subtitle effect colour differs: %1 vs %2", _effect_colour.to_rgb_string(), other->_effect_colour.to_rgb_string()));
		same = false;
	}

	if (_space_before != other->_space_before) {
		note(NoteType::ERROR, String::compose("subtitle space before differs: %1 vs %2", _space_before, other->_space_before));
		same = false;
	}

	if (_rubies != other->_rubies) {
		note(NoteType::ERROR, "rubies differ");
		same = false;
	}

	return same;
}

