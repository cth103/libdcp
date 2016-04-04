/*
    Copyright (C) 2012-2016 Carl Hetherington <cth@carlh.net>

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

/** @file  src/subtitle_string.h
 *  @brief SubtitleString class.
 */

#ifndef LIBDCP_SUBTITLE_STRING_H
#define LIBDCP_SUBTITLE_STRING_H

#include "types.h"
#include "dcp_time.h"
#include <boost/optional.hpp>
#include <string>

namespace dcp {

/** @class SubtitleString
 *  @brief A single line of subtitle text with all the associated attributes.
 */
class SubtitleString
{
public:
	SubtitleString (
		boost::optional<std::string> font,
		bool italic,
		bool bold,
		Colour colour,
		int size,
		float aspect_adjust,
		Time in,
		Time out,
		float h_position,
		HAlign h_align,
		float v_position,
		VAlign v_align,
		std::string text,
		Effect effect,
		Colour effect_colour,
		Time fade_up_time,
		Time fade_down_time
		);

	/** @return font ID */
	boost::optional<std::string> font () const {
		return _font;
	}

	bool italic () const {
		return _italic;
	}

	bool bold () const {
		return _bold;
	}

	Colour colour () const {
		return _colour;
	}

	Time in () const {
		return _in;
	}

	Time out () const {
		return _out;
	}

	std::string text () const {
		return _text;
	}

	float h_position () const {
		return _h_position;
	}

	HAlign h_align () const {
		return _h_align;
	}

	/** @return vertical position as a proportion of the screen height from the
	 *  vertical alignment point.
	 *  (between 0 and 1)
	 */
	float v_position () const {
		return _v_position;
	}

	VAlign v_align () const {
		return _v_align;
	}

	Effect effect () const {
		return _effect;
	}

	Colour effect_colour () const {
		return _effect_colour;
	}

	Time fade_up_time () const {
		return _fade_up_time;
	}

	Time fade_down_time () const {
		return _fade_down_time;
	}

	int size () const {
		return _size;
	}

	int size_in_pixels (int screen_height) const;

	/** @return Aspect ratio `adjustment' of the font size.
	 *  Values greater than 1 widen each character, values less than 1 narrow each character,
	 *  and the value must be between 0.25 and 4.
	 */
	float aspect_adjust () const {
		return _aspect_adjust;
	}

	void set_in (Time i) {
		_in = i;
	}

	void set_out (Time o) {
		_out = o;
	}

	void set_h_position (float p) {
		_h_position = p;
	}

	/** @param p New vertical position as a proportion of the screen height
	 *  from the top (between 0 and 1)
	 */
	void set_v_position (float p) {
		_v_position = p;
	}

	void set_size (int s) {
		_size = s;
	}

	void set_aspect_adjust (float a) {
		_aspect_adjust = a;
	}

private:
	/** font ID */
	boost::optional<std::string> _font;
	/** true if the text is italic */
	bool _italic;
	/** true if the weight is bold, false for normal */
	bool _bold;
	/** text colour */
	Colour _colour;
	/** Size in points as if the screen height is 11 inches, so a 72pt font
	 *  would be 1/11th of the screen height.
	 */
	int _size;
	float _aspect_adjust;
	Time _in;
	Time _out;
	/** Horizontal position as a proportion of the screen width from the _h_align
	 *  (between 0 and 1)
	 */
	float _h_position;
	HAlign _h_align;
	/** Vertical position as a proportion of the screen height from the _v_align
	 *  (between 0 and 1)
	 */
	float _v_position;
	VAlign _v_align;
	std::string _text;
	Effect _effect;
	Colour _effect_colour;
	Time _fade_up_time;
	Time _fade_down_time;
};

bool operator== (SubtitleString const & a, SubtitleString const & b);
std::ostream& operator<< (std::ostream& s, SubtitleString const & sub);

}

#endif
