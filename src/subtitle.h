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


/** @file  src/subtitle.h
 *  @brief Subtitle class
 */


#ifndef LIBDCP_SUBTITLE_H
#define LIBDCP_SUBTITLE_H


#include "dcp_time.h"


namespace dcp {


class Subtitle
{
public:
	virtual ~Subtitle () {}

	/** @return subtitle start time (relative to the start of the reel) */
	Time in () const {
		return _in;
	}

	/** @return subtitle finish time (relative to the start of the reel) */
	Time out () const {
		return _out;
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

	Time fade_up_time () const {
		return _fade_up_time;
	}

	Time fade_down_time () const {
		return _fade_down_time;
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

	void set_fade_up_time (Time t) {
		_fade_up_time = t;
	}

	void set_fade_down_time (Time t) {
		_fade_down_time = t;
	}


protected:

	Subtitle (
		Time in,
		Time out,
		float h_position,
		HAlign h_align,
		float v_position,
		VAlign v_align,
		Time fade_up_time,
		Time fade_down_time
		);

	Time _in;
	Time _out;
	/** Horizontal position as a proportion of the screen width from the _h_align
	 *  (between 0 and 1)
	 */
	float _h_position = 0;
	HAlign _h_align = HAlign::CENTER;
	/** Vertical position as a proportion of the screen height from the _v_align
	 *  (between 0 and 1)
	 */
	float _v_position = 0;
	VAlign _v_align = VAlign::CENTER;
	Time _fade_up_time;
	Time _fade_down_time;
};


}


#endif
