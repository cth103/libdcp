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

#ifndef LIBDCP_SUBTITLE_CONTENT_H
#define LIBDCP_SUBTITLE_CONTENT_H

#include "content.h"
#include "dcp_time.h"
#include "subtitle_string.h"
#include <libcxml/cxml.h>

namespace dcp
{

class SubtitleString;	
class Font;
class Text;
class Subtitle;

/** @class SubtitleContent
 *  @brief A parent for classes representing a file containing subtitles.
 */
class SubtitleContent : public Content
{
public:
	SubtitleContent () {}
	SubtitleContent (boost::filesystem::path file);

	bool equals (
		boost::shared_ptr<const Asset>,
		EqualityOptions,
		boost::function<void (NoteType, std::string)> note
		) const;

	std::string language () const {
		return _language;
	}

	std::list<SubtitleString> subtitles_at (Time t) const;
	std::list<SubtitleString> const & subtitles () const {
		return _subtitles;
	}

	void add (SubtitleString);

	void write_xml (boost::filesystem::path) const;
	virtual Glib::ustring xml_as_string () const {
		/* XXX: this should be pure virtual when SMPTE writing is implemented */
		return "";
	}

	Time latest_subtitle_out () const;

protected:
	void parse_common (boost::shared_ptr<cxml::Document> xml, std::list<boost::shared_ptr<dcp::Font> > font_nodes);
	
	std::string pkl_type (Standard) const {
		return "text/xml";
	}

	std::string asdcp_kind () const {
		return "Subtitle";
	}
	
	/* strangely, this is sometimes a string */
	std::string _reel_number;
	std::string _language;

	std::list<SubtitleString> _subtitles;

private:
	struct ParseState {
		std::list<boost::shared_ptr<Font> > font_nodes;
		std::list<boost::shared_ptr<Text> > text_nodes;
		std::list<boost::shared_ptr<Subtitle> > subtitle_nodes;
	};

	void maybe_add_subtitle (std::string text, ParseState const & parse_state);
	
	void examine_font_nodes (
		boost::shared_ptr<const cxml::Node> xml,
		std::list<boost::shared_ptr<Font> > const & font_nodes,
		ParseState& parse_state
		);
	
	void examine_text_nodes (
		boost::shared_ptr<const cxml::Node> xml,
		std::list<boost::shared_ptr<Text> > const & text_nodes,
		ParseState& parse_state
		);
};

}

#endif
