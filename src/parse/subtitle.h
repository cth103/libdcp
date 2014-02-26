/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#include "../xml.h"
#include "../dcp_time.h"
#include "../types.h"

namespace libdcp
{

namespace parse
{

class Font;

class Text
{
public:
	Text ()
		: v_position (0)
		, v_align (TOP)
	{}
	
	Text (boost::shared_ptr<const cxml::Node> node);

	float v_position;
	VAlign v_align;
	std::string text;
	std::list<boost::shared_ptr<Font> > font_nodes;
};

class Subtitle 
{
public:
	Subtitle () {}
	Subtitle (boost::shared_ptr<const cxml::Node> node);

	Time in;
	Time out;
	Time fade_up_time;
	Time fade_down_time;
	std::list<boost::shared_ptr<Font> > font_nodes;
	std::list<boost::shared_ptr<Text> > text_nodes;

private:
	Time fade_time (boost::shared_ptr<const cxml::Node>, std::string name);
};

class Font 
{
public:
	Font ()
		: size (0)
	{}
	
	Font (boost::shared_ptr<const cxml::Node> node);
	Font (std::list<boost::shared_ptr<Font> > const & font_nodes);

	std::string text;
	std::string id;
	int size;
	boost::optional<bool> italic;
	boost::optional<Color> color;
	boost::optional<Effect> effect;
	boost::optional<Color> effect_color;
	
	std::list<boost::shared_ptr<Subtitle> > subtitle_nodes;
	std::list<boost::shared_ptr<Font> > font_nodes;
	std::list<boost::shared_ptr<Text> > text_nodes;
};

class LoadFont 
{
public:
	LoadFont () {}
	LoadFont (boost::shared_ptr<const cxml::Node> node);

	std::string id;
	boost::optional<std::string> uri;
};

}

}
