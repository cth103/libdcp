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

/** @file  src/parse/cpl.h
 *  @brief Classes used to parse a CPL.
 */

#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <libcxml/cxml.h>
#include "../types.h"

namespace libdcp {

namespace parse	{

/** @brief A simple representation of a CPL \<Picture\> node */
class Picture
{
public:
	Picture () {}
	Picture (boost::shared_ptr<const cxml::Node> node);

	std::string id;
	std::string annotation_text;
	Fraction edit_rate;
	/** Duration of the whole thing */
	int64_t intrinsic_duration;
	/** Start point in frames */
	int64_t entry_point;
	/** Duration that will actually play */
	int64_t duration;
	Fraction frame_rate;
	Fraction screen_aspect_ratio;
	std::string key_id;
};


/** @brief A simple parser for and representation of a CPL \<MainPicture\> node */
class MainPicture : public Picture
{
public:
	MainPicture () {}
	MainPicture (boost::shared_ptr<const cxml::Node> node);
};

/** @brief A simple parser for and representation of a CPL \<MainStereoscopicPicture\> node */
class MainStereoscopicPicture : public Picture
{
public:
	MainStereoscopicPicture () {}
	MainStereoscopicPicture (boost::shared_ptr<const cxml::Node> node);
};

/** @brief A simple parser for and representation of a CPL \<MainSound\> node */
class MainSound
{
public:
	MainSound () {}
	MainSound (boost::shared_ptr<const cxml::Node> node);

	std::string id;
	std::string annotation_text;
	Fraction edit_rate;
	int64_t intrinsic_duration;
	int64_t entry_point;
	int64_t duration;
	std::string key_id;
};

/** @brief A simple parser for and representation of a CPL \<MainSubtitle\> node */
class MainSubtitle
{
public:
	MainSubtitle () {}
	MainSubtitle (boost::shared_ptr<const cxml::Node> node);

	std::string id;
	std::string annotation_text;
	Fraction edit_rate;
	int64_t intrinsic_duration;
	int64_t entry_point;
	int64_t duration;
};

/** @brief A simple parser for and representation of a CPL \<AssetList\> node */
class CPLAssetList
{
public:
	CPLAssetList () {}
	CPLAssetList (boost::shared_ptr<const cxml::Node> node);

	boost::shared_ptr<MainPicture> main_picture;
	boost::shared_ptr<MainStereoscopicPicture> main_stereoscopic_picture;
	boost::shared_ptr<MainSound> main_sound;
	boost::shared_ptr<MainSubtitle> main_subtitle;
};

/** @brief A simple parser for and representation of a CPL \<Reel\> node */
class Reel
{
public:
	Reel () {}
	Reel (boost::shared_ptr<const cxml::Node> node);

	std::string id;
	boost::shared_ptr<CPLAssetList> asset_list;
};


/** @brief A simple parser for and representation of a CPL \<ContentVersion\> node */
class ContentVersion
{
public:
	ContentVersion () {}
	ContentVersion (boost::shared_ptr<const cxml::Node> node);

	std::string id;
	std::string label_text;
};

/** @class CPL
 *  @brief Class to parse a CPL
 *
 *  This class is used to parse XML CPL files.  It is rarely necessary
 *  for the caller to use it outside libdcp.
 */
class CPL
{
public:
	/** Parse a CPL XML file into our member variables */
	CPL (boost::filesystem::path file);

	std::string id;
	std::string annotation_text;
	std::string issue_date;
	std::string creator;
	std::string content_title_text;
	ContentKind content_kind;
	boost::shared_ptr<ContentVersion> content_version;
	std::list<boost::shared_ptr<Reel> > reels;
};

}

}

