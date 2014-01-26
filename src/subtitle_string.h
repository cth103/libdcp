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

#include "types.h"
#include "dcp_time.h"
#include <string>

namespace dcp {

class SubtitleString
{
public:
	SubtitleString (
		std::string font,
		bool italic,
		Color color,
		int size,
		Time in,
		Time out,
		float v_position,
		VAlign v_align,
		std::string text,
		Effect effect,
		Color effect_color,
		Time fade_up_time,
		Time fade_down_time
		);

	std::string font () const {
		return _font;
	}

	bool italic () const {
		return _italic;
	}

	Color color () const {
		return _color;
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

	float v_position () const {
		return _v_position;
	}

	VAlign v_align () const {
		return _v_align;
	}

	Effect effect () const {
		return _effect;
	}

	Color effect_color () const {
		return _effect_color;
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

private:
	std::string _font;
	bool _italic;
	Color _color;
	/** Size in points as if the screen height is 11 inches, so a 72pt font
	 *  would be 1/11th of the screen height.
	 */ 
	int _size;
	Time _in;
	Time _out;
	/** Vertical position as a proportion of the screen height from the top
	 *  (between 0 and 1)
	 */
	float _v_position;
	VAlign _v_align;
	std::string _text;
	Effect _effect;
	Color _effect_color;
	Time _fade_up_time;
	Time _fade_down_time;
};

bool operator== (SubtitleString const & a, SubtitleString const & b);
std::ostream& operator<< (std::ostream& s, SubtitleString const & sub);

}
