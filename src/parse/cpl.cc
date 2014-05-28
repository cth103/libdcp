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
#include "cpl.h"
#include "../xml.h"
#include "../util.h"

using std::string;
using std::bad_cast;
using boost::shared_ptr;
using namespace libdcp::parse;

CPL::CPL (boost::filesystem::path file)
{
	cxml::Document f ("CompositionPlaylist");
	f.read_file (file);
	
	id = f.string_child ("Id");
	annotation_text = f.optional_string_child ("AnnotationText").get_value_or ("");
	issue_date = f.string_child ("IssueDate");
	creator = f.optional_string_child ("Creator").get_value_or ("");
	content_title_text = f.string_child ("ContentTitleText");
	content_kind = content_kind_from_string (f.string_child ("ContentKind"));
	content_version = optional_type_child<ContentVersion> (f, "ContentVersion");
	f.ignore_child ("RatingList");
	reels = type_grand_children<Reel> (f, "ReelList", "Reel");

	f.ignore_child ("Issuer");
	f.ignore_child ("Signer");
	f.ignore_child ("Signature");

	f.done ();
}

ContentVersion::ContentVersion (shared_ptr<const cxml::Node> node)
{
	id = node->optional_string_child ("Id").get_value_or ("");
	label_text = node->string_child ("LabelText");
	node->done ();
}

Reel::Reel (shared_ptr<const cxml::Node> node)
{
	id = node->string_child ("Id");
	asset_list = type_child<CPLAssetList> (node, "AssetList");

	node->ignore_child ("AnnotationText");
	node->done ();
}

CPLAssetList::CPLAssetList (shared_ptr<const cxml::Node> node)
{
	main_picture = optional_type_child<MainPicture> (node, "MainPicture");
	main_stereoscopic_picture = optional_type_child<MainStereoscopicPicture> (node, "MainStereoscopicPicture");
	main_sound = optional_type_child<MainSound> (node, "MainSound");
	main_subtitle = optional_type_child<MainSubtitle> (node, "MainSubtitle");

	node->done ();
}

MainPicture::MainPicture (shared_ptr<const cxml::Node> node)
	: Picture (node)
{

}

MainStereoscopicPicture::MainStereoscopicPicture (shared_ptr<const cxml::Node> node)
	: Picture (node)
{

}

Picture::Picture (shared_ptr<const cxml::Node> node)
{
	id = node->string_child ("Id");
	annotation_text = node->optional_string_child ("AnnotationText").get_value_or ("");
	edit_rate = Fraction (node->string_child ("EditRate"));
	intrinsic_duration = node->number_child<int64_t> ("IntrinsicDuration");
	entry_point = node->number_child<int64_t> ("EntryPoint");
	duration = node->number_child<int64_t> ("Duration");
	frame_rate = Fraction (node->string_child ("FrameRate"));
	try {
		screen_aspect_ratio = Fraction (node->string_child ("ScreenAspectRatio"));
	} catch (XMLError& e) {
		/* Maybe it's not a fraction */
	}
	try {
		float f = node->number_child<float> ("ScreenAspectRatio");
		screen_aspect_ratio = Fraction (f * 1000, 1000);
	} catch (bad_cast& e) {

	}

	key_id = node->optional_string_child ("KeyId").get_value_or ("");

	node->ignore_child ("Hash");

	node->done ();
}

MainSound::MainSound (shared_ptr<const cxml::Node> node)
{
	id = node->string_child ("Id");
	annotation_text = node->optional_string_child ("AnnotationText").get_value_or ("");
	edit_rate = Fraction (node->string_child ("EditRate"));
	intrinsic_duration = node->number_child<int64_t> ("IntrinsicDuration");
	entry_point = node->number_child<int64_t> ("EntryPoint");
	duration = node->number_child<int64_t> ("Duration");
	key_id = node->optional_string_child ("KeyId").get_value_or ("");
	
	node->ignore_child ("Hash");
	node->ignore_child ("Language");
	
	node->done ();
}

MainSubtitle::MainSubtitle (shared_ptr<const cxml::Node> node)
{
	id = node->string_child ("Id");
	annotation_text = node->optional_string_child ("AnnotationText").get_value_or ("");
	edit_rate = Fraction (node->string_child ("EditRate"));
	intrinsic_duration = node->number_child<int64_t> ("IntrinsicDuration");
	entry_point = node->number_child<int64_t> ("EntryPoint");
	duration = node->number_child<int64_t> ("Duration");

	node->ignore_child ("Hash");
	node->ignore_child ("Language");
	
	node->done ();
}
