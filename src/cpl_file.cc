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

/** @file  src/cpl_file.cc
 *  @brief Classes used to parse a CPL.
 */

#include <iostream>
#include "cpl_file.h"

using namespace std;
using namespace libdcp;

CPLFile::CPLFile (string file)
	: XMLFile (file, "CompositionPlaylist")
{
	id = string_child ("Id");
	annotation_text = optional_string_child ("AnnotationText");
	issue_date = string_child ("IssueDate");
	creator = optional_string_child ("Creator");
	content_title_text = string_child ("ContentTitleText");
	content_kind = kind_child ("ContentKind");
	content_version = optional_type_child<ContentVersion> ("ContentVersion");
	ignore_child ("RatingList");
	reels = type_grand_children<CPLReel> ("ReelList", "Reel");

	ignore_child ("Issuer");
	ignore_child ("Signer");
	ignore_child ("Signature");

	done ();
}

ContentVersion::ContentVersion (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = optional_string_child ("Id");
	label_text = string_child ("LabelText");
	done ();
}

CPLReel::CPLReel (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_child ("Id");
	asset_list = type_child<CPLAssetList> ("AssetList");

	ignore_child ("AnnotationText");
	done ();
}

CPLAssetList::CPLAssetList (xmlpp::Node const * node)
	: XMLNode (node)
{
	main_picture = optional_type_child<MainPicture> ("MainPicture");
	main_stereoscopic_picture = optional_type_child<MainStereoscopicPicture> ("MainStereoscopicPicture");
	main_sound = optional_type_child<MainSound> ("MainSound");
	main_subtitle = optional_type_child<MainSubtitle> ("MainSubtitle");

	done ();
}

MainPicture::MainPicture (xmlpp::Node const * node)
	: Picture (node)
{

}

MainStereoscopicPicture::MainStereoscopicPicture (xmlpp::Node const * node)
	: Picture (node)
{

}

Picture::Picture (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_child ("Id");
	annotation_text = optional_string_child ("AnnotationText");
	edit_rate = fraction_child ("EditRate");
	intrinsic_duration = int64_child ("IntrinsicDuration");
	entry_point = int64_child ("EntryPoint");
	duration = int64_child ("Duration");
	frame_rate = fraction_child ("FrameRate");
	try {
		screen_aspect_ratio = fraction_child ("ScreenAspectRatio");
	} catch (XMLError& e) {
		/* Maybe it's not a fraction */
	}
	try {
		float f = float_child ("ScreenAspectRatio");
		screen_aspect_ratio = Fraction (f * 1000, 1000);
	} catch (bad_cast& e) {

	}

	ignore_child ("Hash");

	done ();
}

MainSound::MainSound (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_child ("Id");
	annotation_text = optional_string_child ("AnnotationText");
	edit_rate = fraction_child ("EditRate");
	intrinsic_duration = int64_child ("IntrinsicDuration");
	entry_point = int64_child ("EntryPoint");
	duration = int64_child ("Duration");

	ignore_child ("Hash");
	ignore_child ("Language");
	
	done ();
}

MainSubtitle::MainSubtitle (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_child ("Id");
	annotation_text = optional_string_child ("AnnotationText");
	edit_rate = fraction_child ("EditRate");
	intrinsic_duration = int64_child ("IntrinsicDuration");
	entry_point = int64_child ("EntryPoint");
	duration = int64_child ("Duration");

	ignore_child ("Hash");
	ignore_child ("Language");
	
	done ();
}
