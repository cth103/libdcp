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

class Text : public XMLNode
{
public:
	Text () {}
	Text (xmlpp::Node const * node);

	float v_position () const {
		return _v_position;
	}

	std::string text () const {
		return _text;
	}

private:
	float _v_position;
	std::string _text;
};

class Subtitle : public XMLNode
{
public:
	Subtitle () {}
	Subtitle (xmlpp::Node const * node);

	Time in () const {
		return _in;
	}

	Time out () const {
		return _out;
	}

	std::list<boost::shared_ptr<Text> > const & texts () const {
		return _texts;
	}

private:
	std::list<boost::shared_ptr<Text> > _texts;
	Time _in;
	Time _out;
};

class Font : public XMLNode
{
public:
	Font () {}
	Font (xmlpp::Node const * node);

	std::list<boost::shared_ptr<Subtitle> > const & subtitles () const {
		return _subtitles;
	}

private:
	std::list<boost::shared_ptr<Subtitle> > _subtitles;
};
	
class SubtitleAsset : public Asset, public XMLFile
{
public:
	SubtitleAsset (std::string directory, std::string xml);

	void write_to_cpl (std::ostream&) const {}
	virtual std::list<std::string> equals (boost::shared_ptr<const Asset>, EqualityOptions) const {
		return std::list<std::string> ();
	}

	std::string language () const {
		return _language;
	}

	std::list<boost::shared_ptr<Font> > const & fonts () const {
		return _fonts;
	}

private:
	std::string _subtitle_id;
	std::string _movie_title;
	int64_t _reel_number;
	std::string _language;
	std::list<boost::shared_ptr<Font> > _fonts;
};

}
