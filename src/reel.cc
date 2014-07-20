/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#include "reel.h"
#include "util.h"
#include "picture_mxf.h"
#include "mono_picture_mxf.h"
#include "stereo_picture_mxf.h"
#include "sound_mxf.h"
#include "subtitle_content.h"
#include "reel_mono_picture_asset.h"
#include "reel_stereo_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_subtitle_asset.h"
#include "decrypted_kdm_key.h"
#include "decrypted_kdm.h"
#include <libxml++/nodes/element.h>

using std::string;
using std::list;
using std::cout;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using namespace dcp;

Reel::Reel (boost::shared_ptr<const cxml::Node> node)
	: Object (node->string_child ("Id"))
{
	shared_ptr<cxml::Node> asset_list = node->node_child ("AssetList");

	shared_ptr<cxml::Node> main_picture = asset_list->optional_node_child ("MainPicture");
	if (main_picture) {
		_main_picture.reset (new ReelMonoPictureAsset (main_picture));
	}
	
	shared_ptr<cxml::Node> main_stereoscopic_picture = asset_list->optional_node_child ("MainStereoscopicPicture");
	if (main_stereoscopic_picture) {
		_main_picture.reset (new ReelStereoPictureAsset (main_stereoscopic_picture));
	}
	
	shared_ptr<cxml::Node> main_sound = asset_list->optional_node_child ("MainSound");
	if (main_sound) {
		_main_sound.reset (new ReelSoundAsset (main_sound));
	}
	
	shared_ptr<cxml::Node> main_subtitle = asset_list->optional_node_child ("MainSubtitle");
	if (main_subtitle) {
		_main_subtitle.reset (new ReelSubtitleAsset (main_subtitle));
	}

	node->ignore_child ("AnnotationText");
	node->done ();
}

void
Reel::write_to_cpl (xmlpp::Element* node, Standard standard) const
{
	xmlpp::Element* reel = node->add_child ("Reel");
	reel->add_child("Id")->add_child_text ("urn:uuid:" + make_uuid());
	xmlpp::Element* asset_list = reel->add_child ("AssetList");
	
	if (_main_picture && dynamic_pointer_cast<ReelMonoPictureAsset> (_main_picture)) {
		/* Mono pictures come before other stuff... */
		_main_picture->write_to_cpl (asset_list, standard);
	}

	if (_main_sound) {
		_main_sound->write_to_cpl (asset_list, standard);
	}

	if (_main_subtitle) {
		_main_subtitle->write_to_cpl (asset_list, standard);
	}

	if (_main_picture && dynamic_pointer_cast<ReelStereoPictureAsset> (_main_picture)) {
		/* ... but stereo pictures must come after */
		_main_picture->write_to_cpl (asset_list, standard);
	}
}
	
bool
Reel::equals (boost::shared_ptr<const Reel> other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if ((_main_picture && !other->_main_picture) || (!_main_picture && other->_main_picture)) {
		note (DCP_ERROR, "reel has different assets");
		return false;
	}
	
	if (_main_picture && !_main_picture->equals (other->_main_picture, opt, note)) {
		return false;
	}

	if ((_main_sound && !other->_main_sound) || (!_main_sound && other->_main_sound)) {
		note (DCP_ERROR, "reel has different assets");
		return false;
	}
	
	if (_main_sound && !_main_sound->equals (other->_main_sound, opt, note)) {
		return false;
	}

	if ((_main_subtitle && !other->_main_subtitle) || (!_main_subtitle && other->_main_subtitle)) {
		note (DCP_ERROR, "reel has different assets");
		return false;
	}
	
	if (_main_subtitle && !_main_subtitle->equals (other->_main_subtitle, opt, note)) {
		return false;
	}

	return true;
}

bool
Reel::encrypted () const
{
	return ((_main_picture && _main_picture->encrypted ()) || (_main_sound && _main_sound->encrypted ()));
}

void
Reel::add (DecryptedKDM const & kdm)
{
	list<DecryptedKDMKey> keys = kdm.keys ();

	for (list<DecryptedKDMKey>::iterator i = keys.begin(); i != keys.end(); ++i) {
		if (i->id() == _main_picture->key_id()) {
			_main_picture->mxf()->set_key (i->key ());
		}
		if (i->id() == _main_sound->key_id()) {
			_main_sound->mxf()->set_key (i->key ());
		}
	}
}

void
Reel::add (shared_ptr<ReelAsset> asset)
{
	shared_ptr<ReelPictureAsset> p = dynamic_pointer_cast<ReelPictureAsset> (asset);
	shared_ptr<ReelSoundAsset> so = dynamic_pointer_cast<ReelSoundAsset> (asset);
	shared_ptr<ReelSubtitleAsset> su = dynamic_pointer_cast<ReelSubtitleAsset> (asset);
	if (p) {
		_main_picture = p;
	} else if (so) {
		_main_sound = so;
	} else if (su) {
		_main_subtitle = su;
	}
}

void
Reel::resolve_refs (list<shared_ptr<Object> > objects)
{
	if (_main_picture) {
		_main_picture->content().resolve (objects);
	}

	if (_main_sound) {
		_main_sound->content().resolve (objects);
	}

	if (_main_subtitle) {
		_main_subtitle->content().resolve (objects);
	}
}
