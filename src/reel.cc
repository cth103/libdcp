/*
    Copyright (C) 2014-2017 Carl Hetherington <cth@carlh.net>

    This file is part of libdcp.

    libdcp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libdcp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdcp.  If not, see <http://www.gnu.org/licenses/>.

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/

#include "reel.h"
#include "util.h"
#include "picture_asset.h"
#include "mono_picture_asset.h"
#include "stereo_picture_asset.h"
#include "sound_asset.h"
#include "subtitle_asset.h"
#include "reel_mono_picture_asset.h"
#include "reel_stereo_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_subtitle_asset.h"
#include "reel_markers_asset.h"
#include "decrypted_kdm_key.h"
#include "decrypted_kdm.h"
#include "interop_subtitle_asset.h"
#include "smpte_subtitle_asset.h"
#include "reel_atmos_asset.h"
#include "reel_closed_caption_asset.h"
#include <libxml++/nodes/element.h>
#include <boost/foreach.hpp>

using std::string;
using std::list;
using std::cout;
using std::max;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using namespace dcp;

Reel::Reel (boost::shared_ptr<const cxml::Node> node)
	: Object (remove_urn_uuid (node->string_child ("Id")))
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

	shared_ptr<cxml::Node> main_markers = asset_list->optional_node_child ("MainMarkers");
	if (main_markers) {
		_main_markers.reset (new ReelMarkersAsset (main_markers));
	}

	/* XXX: it's not ideal that we silently tolerate Interop or SMPTE nodes here */
	/* XXX: not sure if Interop supports multiple closed captions */
	list<shared_ptr<cxml::Node> > closed_captions = asset_list->node_children ("MainClosedCaption");
	if (closed_captions.empty()) {
		closed_captions = asset_list->node_children ("ClosedCaption");
	}
	BOOST_FOREACH (shared_ptr<cxml::Node> i, closed_captions) {
		_closed_captions.push_back (shared_ptr<ReelClosedCaptionAsset>(new ReelClosedCaptionAsset(i)));
	}

	shared_ptr<cxml::Node> atmos = asset_list->optional_node_child ("AuxData");
	if (atmos) {
		_atmos.reset (new ReelAtmosAsset (atmos));
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

	if (_main_markers) {
		_main_markers->write_to_cpl (asset_list, standard);
	}

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

	BOOST_FOREACH (shared_ptr<ReelClosedCaptionAsset> i, _closed_captions) {
		i->write_to_cpl (asset_list, standard);
	}

	if (_main_picture && dynamic_pointer_cast<ReelStereoPictureAsset> (_main_picture)) {
		/* ... but stereo pictures must come after */
		_main_picture->write_to_cpl (asset_list, standard);
	}

	if (_atmos) {
		_atmos->write_to_cpl (asset_list, standard);
	}
}

bool
Reel::equals (boost::shared_ptr<const Reel> other, EqualityOptions opt, NoteHandler note) const
{
	if ((_main_picture && !other->_main_picture) || (!_main_picture && other->_main_picture)) {
		note (DCP_ERROR, "Reel: picture assets differ");
		return false;
	}

	if (_main_picture && !_main_picture->equals (other->_main_picture, opt, note)) {
		return false;
	}

	if ((_main_sound && !other->_main_sound) || (!_main_sound && other->_main_sound)) {
		note (DCP_ERROR, "Reel: sound assets differ");
		return false;
	}

	if (_main_sound && !_main_sound->equals (other->_main_sound, opt, note)) {
		return false;
	}

	if ((_main_subtitle && !other->_main_subtitle) || (!_main_subtitle && other->_main_subtitle)) {
		note (DCP_ERROR, "Reel: subtitle assets differ");
		return false;
	}

	if (_main_subtitle && !_main_subtitle->equals (other->_main_subtitle, opt, note)) {
		return false;
	}

	if (_main_markers && !_main_markers->equals (other->_main_markers, opt, note)) {
		return false;
	}

	if (_closed_captions.size() != other->_closed_captions.size()) {
		return false;
	}

	list<shared_ptr<ReelClosedCaptionAsset> >::const_iterator i = _closed_captions.begin();
	list<shared_ptr<ReelClosedCaptionAsset> >::const_iterator j = other->_closed_captions.begin();
	while (i != _closed_captions.end()) {
		if (!(*i)->equals(*j, opt, note)) {
			return false;
		}
		++i;
		++j;
	}

	if ((_atmos && !other->_atmos) || (!_atmos && other->_atmos)) {
		note (DCP_ERROR, "Reel: atmos assets differ");
		return false;
	}

	if (_atmos && !_atmos->equals (other->_atmos, opt, note)) {
		return false;
	}

	return true;
}

bool
Reel::encrypted () const
{
	bool ecc = false;
	BOOST_FOREACH (shared_ptr<ReelClosedCaptionAsset> i, _closed_captions) {
		if (i->encrypted()) {
			ecc = true;
		}
	}

	return (
		(_main_picture && _main_picture->encrypted ()) ||
		(_main_sound && _main_sound->encrypted ()) ||
		(_main_subtitle && _main_subtitle->encrypted ()) ||
		ecc ||
		(_atmos && _atmos->encrypted ())
		);
}

void
Reel::add (DecryptedKDM const & kdm)
{
	list<DecryptedKDMKey> keys = kdm.keys ();

	for (list<DecryptedKDMKey>::iterator i = keys.begin(); i != keys.end(); ++i) {
		if (_main_picture && i->id() == _main_picture->key_id()) {
			_main_picture->asset()->set_key (i->key ());
		}
		if (_main_sound && i->id() == _main_sound->key_id()) {
			_main_sound->asset()->set_key (i->key ());
		}
		if (_main_subtitle && i->id() == _main_subtitle->key_id()) {
			shared_ptr<SMPTESubtitleAsset> s = dynamic_pointer_cast<SMPTESubtitleAsset> (_main_subtitle->asset());
			if (s) {
				s->set_key (i->key ());
			}
		}
		BOOST_FOREACH (shared_ptr<ReelClosedCaptionAsset> j, _closed_captions) {
			if (i->id() == j->key_id()) {
				shared_ptr<SMPTESubtitleAsset> s = dynamic_pointer_cast<SMPTESubtitleAsset> (j->asset());
				if (s) {
					s->set_key (i->key ());
				}
			}
		}
		if (_atmos && i->id() == _atmos->key_id()) {
			_atmos->asset()->set_key (i->key ());
		}
	}
}

void
Reel::add (shared_ptr<ReelAsset> asset)
{
	shared_ptr<ReelPictureAsset> p = dynamic_pointer_cast<ReelPictureAsset> (asset);
	shared_ptr<ReelSoundAsset> so = dynamic_pointer_cast<ReelSoundAsset> (asset);
	shared_ptr<ReelSubtitleAsset> su = dynamic_pointer_cast<ReelSubtitleAsset> (asset);
	shared_ptr<ReelMarkersAsset> m = dynamic_pointer_cast<ReelMarkersAsset> (asset);
	shared_ptr<ReelClosedCaptionAsset> c = dynamic_pointer_cast<ReelClosedCaptionAsset> (asset);
	shared_ptr<ReelAtmosAsset> a = dynamic_pointer_cast<ReelAtmosAsset> (asset);
	if (p) {
		_main_picture = p;
	} else if (so) {
		_main_sound = so;
	} else if (su) {
		_main_subtitle = su;
	} else if (m) {
		_main_markers = m;
	} else if (c) {
		_closed_captions.push_back (c);
	} else if (a) {
		_atmos = a;
	}
}

void
Reel::resolve_refs (list<shared_ptr<Asset> > assets)
{
	if (_main_picture) {
		_main_picture->asset_ref().resolve (assets);
	}

	if (_main_sound) {
		_main_sound->asset_ref().resolve (assets);
	}

	if (_main_subtitle) {
		_main_subtitle->asset_ref().resolve (assets);

		/* Interop subtitle handling is all special cases */
		if (_main_subtitle->asset_ref().resolved()) {
			shared_ptr<InteropSubtitleAsset> iop = dynamic_pointer_cast<InteropSubtitleAsset> (_main_subtitle->asset_ref().asset());
			if (iop) {
				iop->resolve_fonts (assets);
			}
		}
	}

	BOOST_FOREACH (shared_ptr<ReelClosedCaptionAsset> i, _closed_captions) {
		i->asset_ref().resolve(assets);

		/* Interop subtitle handling is all special cases */
		if (i->asset_ref().resolved()) {
			shared_ptr<InteropSubtitleAsset> iop = dynamic_pointer_cast<InteropSubtitleAsset> (i->asset_ref().asset());
			if (iop) {
				iop->resolve_fonts (assets);
			}
		}
	}

	if (_atmos) {
		_atmos->asset_ref().resolve (assets);
	}
}

int64_t
Reel::duration () const
{
	int64_t d = 0;

	if (_main_picture) {
		d = max (d, _main_picture->actual_duration());
	}
	if (_main_sound) {
		d = max (d, _main_sound->actual_duration());
	}
	if (_main_subtitle) {
		d = max (d, _main_subtitle->actual_duration());
	}
	if (_main_markers) {
		d = max (d, _main_markers->actual_duration());
	}
	BOOST_FOREACH (shared_ptr<ReelClosedCaptionAsset> i, _closed_captions) {
		d = max (d, i->actual_duration());
	}
	if (_atmos) {
		d = max (d, _atmos->actual_duration());
	}

	return d;
}
