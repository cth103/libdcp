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


/** @file  src/subtitle_string.h
 *  @brief SubtitleString class
 */


#ifndef LIBDCP_SUBTITLE_STRING_H
#define LIBDCP_SUBTITLE_STRING_H


#include "types.h"
#include "subtitle.h"
#include "dcp_time.h"
#include <boost/optional.hpp>
#include <string>


namespace dcp {


/** @class SubtitleString
 *  @brief A single line of subtitle text with all the associated attributes.
 */
class SubtitleString : public Subtitle
{
public:
	/** @param font Font ID, or empty to use the default
	 *  @param italic true for italic text
	 *  @param bold true for bold text
	 *  @param underline true for underlined text
	 *  @param colour Colour of the text
	 *  @param size Size in points as if the screen height is 11 inches, so a 72pt font would be 1/11th of the screen height
	 *  @param aspect_adjust greater than 1 to stretch text to be wider, less than 1 to shrink text to be narrower (must be between 0.25 and 4)
	 *  @param in start time
	 *  @param out finish time
	 *  @param h_position Horizontal position as a fraction of the screen width (between 0 and 1) from h_align
	 *  @param h_align Horizontal alignment point
	 *  @param v_position Vertical position as a fraction of the screen height (between 0 and 1) from v_align
	 *  @param v_align Vertical alignment point
	 *  @param direction Direction of text
	 *  @param text The text to display
	 *  @param effect Effect to use
	 *  @param effect_colour Colour of the effect
	 *  @param fade_up_time Time to fade the text in
	 *  @param fade_down_time Time to fade the text out
	 */
	SubtitleString (
		boost::optional<std::string> font,
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

	bool underline () const {
		return _underline;
	}

	Colour colour () const {
		return _colour;
	}

	std::string text () const {
		return _text;
	}

	Direction direction () const {
		return _direction;
	}

	Effect effect () const {
		return _effect;
	}

	Colour effect_colour () const {
		return _effect_colour;
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

	void set_font (std::string id) {
		_font = id;
	}

	void unset_font () {
		_font = boost::optional<std::string>();
	}

	void set_size (int s) {
		_size = s;
	}

	void set_aspect_adjust (float a) {
		_aspect_adjust = a;
	}

	void set_text (std::string t) {
		_text = t;
	}

	void set_colour (Colour c) {
		_colour = c;
	}

	void set_effect (Effect e) {
		_effect = e;
	}

	void set_effect_colour (Colour c) {
		_effect_colour = c;
	}

private:
	/** font ID */
	boost::optional<std::string> _font;
	/** true if the text is italic */
	bool _italic;
	/** true if the weight is bold, false for normal */
	bool _bold;
	/** true to enable underlining, false otherwise */
	bool _underline;
	/** text colour */
	Colour _colour;
	/** Size in points as if the screen height is 11 inches, so a 72pt font
	 *  would be 1/11th of the screen height.
	 */
	int _size;
	float _aspect_adjust;
	Direction _direction;
	std::string _text;
	Effect _effect;
	Colour _effect_colour;
};

bool operator== (SubtitleString const & a, SubtitleString const & b);
bool operator!= (SubtitleString const & a, SubtitleString const & b);
std::ostream& operator<< (std::ostream& s, SubtitleString const & sub);


}


#endif
