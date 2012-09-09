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

/** @file  src/cpl_file.h
 *  @brief Classes used to parse a CPL.
 */

#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include "xml.h"

namespace libdcp {

class Picture : public XMLNode
{
public:
	Picture () {}
	Picture (xmlpp::Node const * node);

	std::string id;
	std::string annotation_text;
	Fraction edit_rate;
	int64_t intrinsic_duration;
	int64_t entry_point;
	int64_t duration;
	Fraction frame_rate;
	Fraction screen_aspect_ratio;
};


/** CPL MainPicture node */
class MainPicture : public Picture
{
public:
	MainPicture () {}
	MainPicture (xmlpp::Node const * node);
};

/** CPL MainStereoscopicPicture node */
class MainStereoscopicPicture : public Picture
{
public:
	MainStereoscopicPicture () {}
	MainStereoscopicPicture (xmlpp::Node const * node);
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
	boost::shared_ptr<MainStereoscopicPicture> main_stereoscopic_picture;
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

/** @class CPLFile
 *  @brief Class to parse a CPL
 *
 *  This class is used to parse XML CPL files.  It is rarely necessary
 *  for the caller to use it outside libdcp.
 */
class CPLFile : public XMLFile
{
public:
	/** Parse a CPL XML file into our member variables */
	CPLFile (std::string file);

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

