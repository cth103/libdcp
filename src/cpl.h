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

/** @file  src/cpl.h
 *  @brief Classes used to parse a CPL.
 */

#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include "xml.h"

namespace libdcp {

/** CPL MainPicture node */	
class MainPicture : public XMLNode
{
public:
	MainPicture () {}
	MainPicture (xmlpp::Node const * node);

	std::string id;
	std::string annotation_text;
	Fraction edit_rate;
	int64_t intrinsic_duration;
	int64_t entry_point;
	int64_t duration;
	Fraction frame_rate;
	Fraction screen_aspect_ratio;
};

/** CPL MainSound node */	
class MainSound : public XMLNode
{
public:
	MainSound () {}
	MainSound (xmlpp::Node const * node);

	std::string id;
	std::string annotation_text;
	Fraction edit_rate;
	int64_t intrinsic_duration;
	int64_t entry_point;
	int64_t duration;
};

/** CPL MainSubtitle node */	
class MainSubtitle : public XMLNode
{
public:
	MainSubtitle () {}
	MainSubtitle (xmlpp::Node const * node);

	std::string id;
	std::string annotation_text;
	Fraction edit_rate;
	int64_t intrinsic_duration;
	int64_t entry_point;
	int64_t duration;
};

/** CPL AssetList node */	
class CPLAssetList : public XMLNode
{
public:
	CPLAssetList () {}
	CPLAssetList (xmlpp::Node const * node);

	boost::shared_ptr<MainPicture> main_picture;
	boost::shared_ptr<MainSound> main_sound;
	boost::shared_ptr<MainSubtitle> main_subtitle;
};

/** CPL Reel node */	
class CPLReel : public XMLNode
{
public:
	CPLReel () {}
	CPLReel (xmlpp::Node const * node);

	std::string id;
	boost::shared_ptr<CPLAssetList> asset_list;
};

/** CPL ContentVersion node */	
class ContentVersion : public XMLNode
{
public:
	ContentVersion () {}
	ContentVersion (xmlpp::Node const * node);

	std::string id;
	std::string label_text;
};

/** Class to parse a CPL */
class CPL : public XMLFile
{
public:
	/** Parse a CPL XML file into our member variables */
	CPL (std::string file);

	std::string id;
	std::string annotation_text;
	std::string issue_date;
	std::string creator;
	std::string content_title_text;
	ContentKind content_kind;
	boost::shared_ptr<ContentVersion> content_version;
	std::list<boost::shared_ptr<CPLReel> > reels;
};

}

