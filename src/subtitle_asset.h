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

#include "asset.h"
#include "xml.h"
#include "dcp_time.h"

namespace libdcp
{

class TextNode : public XMLNode
{
public:
	TextNode () {}
	TextNode (xmlpp::Node const * node);

	float v_position;
	std::string text;
};

class SubtitleNode : public XMLNode
{
public:
	SubtitleNode () {}
	SubtitleNode (xmlpp::Node const * node);

	Time in;
	Time out;
	std::list<boost::shared_ptr<TextNode> > text_nodes;
};

class FontNode : public XMLNode
{
public:
	FontNode () {}
	FontNode (xmlpp::Node const * node);

	std::string id;
	int size;
	
	std::list<boost::shared_ptr<SubtitleNode> > subtitle_nodes;
};

class LoadFontNode : public XMLNode
{
public:
	LoadFontNode () {}
	LoadFontNode (xmlpp::Node const * node);

	std::string id;
	std::string uri;
};

class Subtitle
{
public:
	Subtitle (
		std::string font,
		int size,
		Time in,
		Time out,
		float v_position,
		std::string text
		);

	std::string font () const {
		return _font;
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

	int size_in_pixels (int screen_height) const;

private:
	std::string _font;
	int _size;
	Time _in;
	Time _out;
	float _v_position;
	std::string _text;
};

class SubtitleAsset : public Asset, public XMLFile
{
public:
	SubtitleAsset (std::string directory, std::string xml);

	void write_to_cpl (std::ostream&) const {}
	virtual std::list<std::string> equals (boost::shared_ptr<const Asset>, EqualityOptions) const {
		/* XXX */
		return std::list<std::string> ();
	}

	std::string language () const {
		return _language;
	}

	std::list<boost::shared_ptr<Subtitle> > subtitles_at (Time t) const;

private:
	std::string font_id_to_name (std::string id, std::list<boost::shared_ptr<LoadFontNode> > const & load_font_nodes) const;
	
	std::string _subtitle_id;
	std::string _movie_title;
	int64_t _reel_number;
	std::string _language;

	std::list<boost::shared_ptr<Subtitle> > _subtitles;
};

}
