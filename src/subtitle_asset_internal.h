/*
    Copyright (C) 2012-2016 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_SUBTITLE_ASSET_INTERNAL_H
#define LIBDCP_SUBTITLE_ASSET_INTERNAL_H

#include "raw_convert.h"
#include "types.h"
#include "dcp_time.h"
#include <libxml++/libxml++.h>
#include <boost/foreach.hpp>

struct take_intersection_test;
struct take_difference_test;
struct pull_fonts_test1;
struct pull_fonts_test2;
struct pull_fonts_test3;

namespace dcp {

class SubtitleString;

namespace order {

struct Context
{
	std::string xmlns () const;

	int time_code_rate;
	Standard standard;
	int spot_number;
};

class Font
{
public:
	Font () {}

	Font (SubtitleString const & s, Standard standard);

	xmlpp::Element* as_xml (xmlpp::Element* parent, Context& context) const;

	void take_intersection (Font other);
	void take_difference (Font other);
	bool empty () const;
	void clear ();
	bool operator== (Font const & other) const;

private:
	friend struct ::take_intersection_test;
	friend struct ::take_difference_test;
	friend struct ::pull_fonts_test1;
	friend struct ::pull_fonts_test2;
	friend struct ::pull_fonts_test3;

	std::map<std::string, std::string> _values;
};

class Part
{
public:
	Part (boost::shared_ptr<Part> parent_)
		: parent (parent_)
	{}

	Part (boost::shared_ptr<Part> parent_, Font font_)
		: parent (parent_)
		, font (font_)
	{}

	virtual xmlpp::Element* as_xml (xmlpp::Element* parent, Context &) const;
	void write_xml (xmlpp::Element* parent, order::Context& context) const;

	boost::shared_ptr<Part> parent;
	Font font;
	std::list<boost::shared_ptr<Part> > children;
};

class String : public Part
{
public:
	String (boost::shared_ptr<Part> parent, Font font, std::string text_)
		: Part (parent, font)
		, text (text_)
	{}

	virtual xmlpp::Element* as_xml (xmlpp::Element* parent, Context &) const;

	std::string text;
};

class Text : public Part
{
public:
	Text (boost::shared_ptr<Part> parent, HAlign h_align, float h_position, VAlign v_align, float v_position, Direction direction)
		: Part (parent)
		, _h_align (h_align)
		, _h_position (h_position)
		, _v_align (v_align)
		, _v_position (v_position)
		, _direction (direction)
	{}

	xmlpp::Element* as_xml (xmlpp::Element* parent, Context& context) const;

private:
	HAlign _h_align;
	float _h_position;
	VAlign _v_align;
	float _v_position;
	Direction _direction;
};

class Subtitle : public Part
{
public:
	Subtitle (boost::shared_ptr<Part> parent, Time in, Time out, Time fade_up, Time fade_down)
		: Part (parent)
		, _in (in)
		, _out (out)
		, _fade_up (fade_up)
		, _fade_down (fade_down)
	{}

	xmlpp::Element* as_xml (xmlpp::Element* parent, Context& context) const;

private:
	Time _in;
	Time _out;
	Time _fade_up;
	Time _fade_down;
};

}
}

#endif
