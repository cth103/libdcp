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

#include "subtitle_asset.h"
#include "local_time.h"
#include "mxf.h"
#include <boost/filesystem.hpp>

namespace dcp {

class SMPTELoadFontNode;

class SMPTESubtitleAsset : public SubtitleAsset, public MXF
{
public:
	/** @param file File name
	 *  @param mxf true if `file' is a MXF, or false if it is an XML file.
	 */
	SMPTESubtitleAsset (boost::filesystem::path file, bool mxf = true);

	bool equals (
		boost::shared_ptr<const Asset>,
		EqualityOptions,
		NoteHandler note
		) const;
	
	std::list<boost::shared_ptr<LoadFontNode> > load_font_nodes () const;

	Glib::ustring xml_as_string () const;
	void write (boost::filesystem::path path) const;

	/** @return language, if one was specified */
	boost::optional<std::string> language () const {
		return _language;
	}
	
	static bool valid_mxf (boost::filesystem::path);

protected:
	
	std::string pkl_type (Standard) const {
		return "application/mxf";
	}
	
private:
	std::string _content_title_text;
	boost::optional<std::string> _annotation_text;
	LocalTime _issue_date;
	boost::optional<int> _reel_number;
	boost::optional<std::string> _language;
	Fraction _edit_rate;
	int _time_code_rate;
	boost::optional<Time> _start_time;
	
	std::list<boost::shared_ptr<SMPTELoadFontNode> > _load_font_nodes;
};

}
