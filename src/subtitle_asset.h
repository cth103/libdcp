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

#ifndef LIBDCP_SUBTITLE_ASSET_H
#define LIBDCP_SUBTITLE_ASSET_H

#include <libcxml/cxml.h>
#include "asset.h"
#include "dcp_time.h"

namespace libdcp
{

namespace parse
{
	class Font;
	class Text;
	class Subtitle;
	class LoadFont;
}

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

	void set_text (std::string t) {
		_text = t;
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

bool operator== (Subtitle const & a, Subtitle const & b);
std::ostream& operator<< (std::ostream& s, Subtitle const & sub);

class SubtitleAsset : public Asset
{
public:
	SubtitleAsset (std::string directory, std::string xml_file);
	SubtitleAsset (std::string directory, std::string movie_title, std::string language);

	void write_to_cpl (xmlpp::Element *) const;
	virtual bool equals (boost::shared_ptr<const Asset>, EqualityOptions, boost::function<void (NoteType, std::string)> note) const {
		/* XXX */
		note (ERROR, "subtitle assets not compared yet");
		return true;
	}

	std::string language () const {
		return _language;
	}

	std::list<boost::shared_ptr<Subtitle> > subtitles_at (Time t) const;
	std::list<boost::shared_ptr<Subtitle> > const & subtitles () const {
		return _subtitles;
	}

	void add (boost::shared_ptr<Subtitle>);

	void read_xml (std::string);
	void write_xml () const;
	Glib::ustring xml_as_string () const;

protected:

	std::string asdcp_kind () const {
		return "Subtitle";
	}

private:
	std::string font_id_to_name (std::string id) const;
	void read_mxf (std::string);
	void read_xml (boost::shared_ptr<cxml::Document>);

	struct ParseState {
		std::list<boost::shared_ptr<parse::Font> > font_nodes;
		std::list<boost::shared_ptr<parse::Text> > text_nodes;
		std::list<boost::shared_ptr<parse::Subtitle> > subtitle_nodes;
	};

	void maybe_add_subtitle (std::string text, ParseState const & parse_state);
	
	void examine_font_nodes (
		boost::shared_ptr<const cxml::Node> xml,
		std::list<boost::shared_ptr<parse::Font> > const & font_nodes,
		ParseState& parse_state
		);
	
	void examine_text_nodes (
		boost::shared_ptr<const cxml::Node> xml,
		std::list<boost::shared_ptr<parse::Text> > const & text_nodes,
		ParseState& parse_state
		);

	boost::optional<std::string> _movie_title;
	/* strangely, this is sometimes a string */
	std::string _reel_number;
	std::string _language;
	std::list<boost::shared_ptr<parse::LoadFont> > _load_font_nodes;

	std::list<boost::shared_ptr<Subtitle> > _subtitles;
	bool _need_sort;
};

}

#endif
