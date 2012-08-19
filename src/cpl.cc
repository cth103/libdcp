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

/** @file  src/cpl.cc
 *  @brief Classes used to parse a CPL.
 */

#include <iostream>
#include "cpl.h"

using namespace std;
using namespace libdcp;

CPL::CPL (string file)
	: XMLFile (file, "CompositionPlaylist")
{
	id = string_node ("Id");
	annotation_text = optional_string_node ("AnnotationText");
	issue_date = string_node ("IssueDate");
	creator = string_node ("Creator");
	content_title_text = string_node ("ContentTitleText");
	content_kind = kind_node ("ContentKind");
	content_version = optional_sub_node<ContentVersion> ("ContentVersion");
	ignore_node ("RatingList");
	reels = sub_nodes<CPLReel> ("ReelList", "Reel");

	ignore_node ("Issuer");
	ignore_node ("Signer");
	ignore_node ("Signature");

	done ();
}

ContentVersion::ContentVersion (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_node ("Id");
	label_text = string_node ("LabelText");
	done ();
}

CPLReel::CPLReel (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_node ("Id");
	asset_list = sub_node<CPLAssetList> ("AssetList");

	done ();
}

CPLAssetList::CPLAssetList (xmlpp::Node const * node)
	: XMLNode (node)
{
	main_picture = sub_node<MainPicture> ("MainPicture");
	main_sound = optional_sub_node<MainSound> ("MainSound");
	main_subtitle = optional_sub_node<MainSubtitle> ("MainSubtitle");

	done ();
}

MainPicture::MainPicture (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_node ("Id");
	annotation_text = optional_string_node ("AnnotationText");
	edit_rate = fraction_node ("EditRate");
	intrinsic_duration = int64_node ("IntrinsicDuration");
	entry_point = int64_node ("EntryPoint");
	duration = int64_node ("Duration");
	frame_rate = fraction_node ("FrameRate");
	try {
		screen_aspect_ratio = fraction_node ("ScreenAspectRatio");
	} catch (XMLError& e) {
		/* Maybe it's not a fraction */
	}
	try {
		float f = float_node ("ScreenAspectRatio");
		screen_aspect_ratio = Fraction (f * 1000, 1000);
	} catch (bad_cast& e) {

	}

	ignore_node ("Hash");

	done ();
}

MainSound::MainSound (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_node ("Id");
	annotation_text = optional_string_node ("AnnotationText");
	edit_rate = fraction_node ("EditRate");
	intrinsic_duration = int64_node ("IntrinsicDuration");
	entry_point = int64_node ("EntryPoint");
	duration = int64_node ("Duration");

	ignore_node ("Hash");
	
	done ();
}

MainSubtitle::MainSubtitle (xmlpp::Node const * node)
	: XMLNode (node)
{
	id = string_node ("Id");
	annotation_text = optional_string_node ("AnnotationText");
	edit_rate = fraction_node ("EditRate");
	intrinsic_duration = int64_node ("IntrinsicDuration");
	entry_point = int64_node ("EntryPoint");
	duration = int64_node ("Duration");

	ignore_node ("Hash");
	
	done ();
}
