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

#include <libxml++/nodes/element.h>
#include "reel.h"
#include "util.h"
#include "picture_asset.h"
#include "mono_picture_asset.h"
#include "stereo_picture_asset.h"
#include "sound_asset.h"
#include "subtitle_asset.h"
#include "kdm.h"

using std::string;
using std::list;
using std::cout;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using namespace libdcp;

void
Reel::write_to_cpl (xmlpp::Element* node) const
{
	xmlpp::Element* reel = node->add_child ("Reel");
	reel->add_child("Id")->add_child_text ("urn:uuid:" + make_uuid());
	xmlpp::Element* asset_list = reel->add_child ("AssetList");
	
	if (_main_picture && dynamic_pointer_cast<MonoPictureAsset> (_main_picture)) {
		/* Mono pictures come before other stuff... */
		_main_picture->write_to_cpl (asset_list);
	}

	if (_main_sound) {
		_main_sound->write_to_cpl (asset_list);
	}

	if (_main_subtitle) {
		_main_subtitle->write_to_cpl (asset_list);
	}

	if (_main_picture && dynamic_pointer_cast<StereoPictureAsset> (_main_picture)) {
		/* ... but stereo pictures must come after */
		_main_picture->write_to_cpl (asset_list);
	}
}
	
bool
Reel::equals (boost::shared_ptr<const Reel> other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if ((_main_picture && !other->_main_picture) || (!_main_picture && other->_main_picture)) {
		note (ERROR, "reel has different assets");
		return false;
	}
	
	if (_main_picture && !_main_picture->equals (other->_main_picture, opt, note)) {
		return false;
	}

	if ((_main_sound && !other->_main_sound) || (!_main_sound && other->_main_sound)) {
		note (ERROR, "reel has different assets");
		return false;
	}
	
	if (_main_sound && !_main_sound->equals (other->_main_sound, opt, note)) {
		return false;
	}

	if ((_main_subtitle && !other->_main_subtitle) || (!_main_subtitle && other->_main_subtitle)) {
		note (ERROR, "reel has different assets");
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
Reel::add_kdm (KDM const & kdm)
{
	list<KDMKey> keys = kdm.keys ();
	
	for (list<KDMKey>::iterator i = keys.begin(); i != keys.end(); ++i) {
		if (i->key_id() == _main_picture->key_id()) {
			_main_picture->set_key (i->key ());
		}
		if (i->key_id() == _main_sound->key_id()) {
			_main_sound->set_key (i->key ());
		}
	}
}

void
Reel::set_mxf_keys (Key key)
{
	_main_picture->set_key (key);
	if (_main_sound) {
		_main_sound->set_key (key);
	}

	/* XXX: subtitle asset? */
}
