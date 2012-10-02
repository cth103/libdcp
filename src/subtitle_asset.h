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

class FontNode;

class TextNode : public XMLNode
{
public:
	TextNode () {}
	TextNode (xmlpp::Node const * node);

	float v_position;
	VAlign v_align;
	std::string text;
	std::list<boost::shared_ptr<FontNode> > font_nodes;
};

class SubtitleNode : public XMLNode
{
public:
	SubtitleNode () {}
	SubtitleNode (xmlpp::Node const * node);

	Time in;
	Time out;
	Time fade_up_time;
	Time fade_down_time;
	std::list<boost::shared_ptr<FontNode> > font_nodes;
	std::list<boost::shared_ptr<TextNode> > text_nodes;

private:
	Time fade_time (std::string name);
};

class FontNode : public XMLNode
{
public:
	FontNode () {}
	FontNode (xmlpp::Node const * node);
	FontNode (std::list<boost::shared_ptr<FontNode> > const & font_nodes);

	std::string text;
	std::string id;
	int size;
	boost::optional<bool> italic;
	boost::optional<Color> color;
	boost::optional<Effect> effect;
	boost::optional<Color> effect_color;
	
	std::list<boost::shared_ptr<SubtitleNode> > subtitle_nodes;
	std::list<boost::shared_ptr<FontNode> > font_nodes;
	std::list<boost::shared_ptr<TextNode> > text_nodes;
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
	int _size;
	Time _in;
	Time _out;
	float _v_position;
	VAlign _v_align;
	std::string _text;
	Effect _effect;
	Color _effect_color;
	Time _fade_up_time;
	Time _fade_down_time;
};

bool operator== (Subtitle const & a, Subtitle const & b);
std::ostream& operator<< (std::ostream& s, Subtitle const & sub);

class SubtitleAsset : public Asset, public XMLFile
{
public:
	SubtitleAsset (std::string directory, std::string xml);

	void write_to_cpl (std::ostream&) const {}
	virtual bool equals (boost::shared_ptr<const Asset>, EqualityOptions, std::list<std::string>& notes) const {
		/* XXX */
		notes.push_back ("subtitle assets not compared yet");
		return true;
	}

	std::string language () const {
		return _language;
	}

	std::list<boost::shared_ptr<Subtitle> > subtitles_at (Time t) const;
	std::list<boost::shared_ptr<Subtitle> > const & subtitles () const {
		return _subtitles;
	}

private:
	std::string font_id_to_name (std::string id) const;

	struct ParseState {
		std::list<boost::shared_ptr<FontNode> > font_nodes;
		std::list<boost::shared_ptr<TextNode> > text_nodes;
		std::list<boost::shared_ptr<SubtitleNode> > subtitle_nodes;
	};

	void maybe_add_subtitle (std::string text, ParseState const & parse_state);
	
	void examine_font_nodes (
		std::list<boost::shared_ptr<FontNode> > const & font_nodes,
		ParseState& parse_state
		);
	
	void examine_text_nodes (
		std::list<boost::shared_ptr<TextNode> > const & text_nodes,
		ParseState& parse_state
		);
	
	std::string _subtitle_id;
	std::string _movie_title;
	/* strangely, this is sometimes a string */
	std::string _reel_number;
	std::string _language;
	std::list<boost::shared_ptr<LoadFontNode> > _load_font_nodes;

	std::list<boost::shared_ptr<Subtitle> > _subtitles;
};

}
