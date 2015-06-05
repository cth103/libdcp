/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

/** @file  src/interop_subtitle_asset.h
 *  @brief InteropSubtitleAsset class.
 */

#include "subtitle_asset.h"
#include <boost/filesystem.hpp>

namespace dcp {

class InteropLoadFontNode;

/** @class InteropSubtitleAsset
 *  @brief A set of subtitles to be read and/or written in the Inter-Op format.
 *
 *  Inter-Op subtitles are sometimes known as CineCanvas.
 */
class InteropSubtitleAsset : public SubtitleAsset
{
public:
	InteropSubtitleAsset (std::string movie_title, std::string language);
	InteropSubtitleAsset (boost::filesystem::path file);

	bool equals (
		boost::shared_ptr<const Asset>,
		EqualityOptions,
		NoteHandler note
		) const;

	std::list<boost::shared_ptr<LoadFontNode> > load_font_nodes () const;

	void add_font (std::string id, std::string uri);
	
	Glib::ustring xml_as_string () const;
	void write (boost::filesystem::path path) const;

	/** Set the reel number or sub-element identifier
	 *  of these subtitles.
	 *  @param n New reel number.
	 */
	void set_reel_number (std::string n) {
		_reel_number = n;
	}

	/** Set the language tag of these subtitles.
	 *  @param l New language.
	 */
	void set_language (std::string l) {
		_language = l;
	}

	/** @return title of the movie that the subtitles are for */
	void set_movie_title (std::string m) {
		_movie_title = m;
	}

	/** @return reel number or sub-element of a programme that
	 *  these subtitles refer to.
	 */
	std::string reel_number () const {
		return _reel_number;
	}

	/** @return language used in the subtitles */
	std::string language () const {
		return _language;
	}

	/** @return movie title that these subtitles are for */
	std::string movie_title () const {
		return _movie_title;
	}

protected:
	
	std::string pkl_type (Standard) const {
		return "text/xml";
	}

private:
	std::string _reel_number;
	std::string _language;
	std::string _movie_title;
	std::list<boost::shared_ptr<InteropLoadFontNode> > _load_font_nodes;
};

}
