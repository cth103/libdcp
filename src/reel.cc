/*
    Copyright (C) 2014-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/reel.cc
 *  @brief Reel class
 */


#include "decrypted_kdm.h"
#include "decrypted_kdm_key.h"
#include "equality_options.h"
#include "interop_text_asset.h"
#include "mono_j2k_picture_asset.h"
#include "j2k_picture_asset.h"
#include "reel.h"
#include "reel_atmos_asset.h"
#include "reel_interop_text_asset.h"
#include "reel_markers_asset.h"
#include "reel_mono_picture_asset.h"
#include "reel_smpte_text_asset.h"
#include "reel_sound_asset.h"
#include "reel_stereo_picture_asset.h"
#include "reel_text_asset.h"
#include "smpte_text_asset.h"
#include "sound_asset.h"
#include "stereo_j2k_picture_asset.h"
#include "text_asset.h"
#include "util.h"
#include <libxml++/nodes/element.h>
#include <stdint.h>


using std::string;
using std::cout;
using std::min;
using std::make_shared;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using std::vector;
using namespace dcp;


Reel::Reel (std::shared_ptr<const cxml::Node> node, dcp::Standard standard)
	: Object (remove_urn_uuid (node->string_child ("Id")))
{
	auto asset_list = node->node_child ("AssetList");

	if (auto main_picture = asset_list->optional_node_child("MainPicture")) {
		_main_picture = make_shared<ReelMonoPictureAsset>(main_picture);
	}

	if (auto main_stereoscopic_picture = asset_list->optional_node_child("MainStereoscopicPicture")) {
		_main_picture = make_shared<ReelStereoPictureAsset>(main_stereoscopic_picture);
	}

	if (auto main_sound = asset_list->optional_node_child("MainSound")) {
		_main_sound = make_shared<ReelSoundAsset>(main_sound);
	}

	if (auto main_subtitle = asset_list->optional_node_child("MainSubtitle")) {
		switch (standard) {
		case Standard::INTEROP:
			_main_subtitle = make_shared<ReelInteropTextAsset>(main_subtitle);
			break;
		case Standard::SMPTE:
			_main_subtitle = make_shared<ReelSMPTETextAsset>(main_subtitle);
			break;
		}
	}

	if (auto main_caption = asset_list->optional_node_child("MainCaption")) {
		switch (standard) {
		case Standard::INTEROP:
			DCP_ASSERT(false);
			break;
		case Standard::SMPTE:
			_main_caption = make_shared<ReelSMPTETextAsset>(main_caption);
			break;
		}
	}

	if (auto main_markers = asset_list->optional_node_child("MainMarkers")) {
		_main_markers = make_shared<ReelMarkersAsset>(main_markers);
	}

	auto closed_subtitles = asset_list->node_children("ClosedSubtitle");
	for (auto i: closed_subtitles) {
		switch (standard) {
		case Standard::INTEROP:
			DCP_ASSERT(false);
			break;
		case Standard::SMPTE:
			_closed_subtitles.push_back(make_shared<ReelSMPTETextAsset>(i));
			break;
		}
	}

	/* XXX: it's not ideal that we silently tolerate Interop or SMPTE nodes here */
	/* XXX: not sure if Interop supports multiple closed captions */
	auto closed_captions = asset_list->node_children ("MainClosedCaption");
	if (closed_captions.empty()) {
		closed_captions = asset_list->node_children ("ClosedCaption");
	}
	for (auto i: closed_captions) {
		switch (standard) {
		case Standard::INTEROP:
			_closed_captions.push_back(make_shared<ReelInteropTextAsset>(i));
			break;
		case Standard::SMPTE:
			_closed_captions.push_back(make_shared<ReelSMPTETextAsset>(i));
			break;
		}
	}

	if (auto atmos = asset_list->optional_node_child("AuxData")) {
		_atmos = make_shared<ReelAtmosAsset>(atmos);
	}

	node->ignore_child ("AnnotationText");
	node->done ();
}


xmlpp::Element *
Reel::write_to_cpl (xmlpp::Element* node, Standard standard) const
{
	auto reel = cxml::add_child(node, "Reel");
	cxml::add_text_child(reel, "Id", "urn:uuid:" + _id);
	auto asset_list = cxml::add_child(reel, "AssetList");

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

	if (_main_caption) {
		_main_caption->write_to_cpl(asset_list, standard);
	}

	for (auto i: _closed_subtitles) {
		i->write_to_cpl(asset_list, standard);
	}

	for (auto i: _closed_captions) {
		i->write_to_cpl (asset_list, standard);
	}

	if (_main_picture && dynamic_pointer_cast<ReelStereoPictureAsset> (_main_picture)) {
		/* ... but stereo pictures must come after */
		_main_picture->write_to_cpl (asset_list, standard);
	}

	if (_atmos) {
		_atmos->write_to_cpl (asset_list, standard);
	}

	return asset_list;
}


bool
Reel::equals(std::shared_ptr<const Reel> other, EqualityOptions const& opt, NoteHandler note) const
{
	if ((_main_picture && !other->_main_picture) || (!_main_picture && other->_main_picture)) {
		note (NoteType::ERROR, "Reel: picture assets differ");
		return false;
	}

	if (_main_picture && !_main_picture->equals (other->_main_picture, opt, note)) {
		return false;
	}

	if ((_main_sound && !other->_main_sound) || (!_main_sound && other->_main_sound)) {
		note (NoteType::ERROR, "Reel: sound assets differ");
		return false;
	}

	if (_main_sound && !_main_sound->equals (other->_main_sound, opt, note)) {
		return false;
	}

	if ((_main_subtitle && !other->_main_subtitle) || (!_main_subtitle && other->_main_subtitle)) {
		note (NoteType::ERROR, "Reel: subtitle assets differ");
		return false;
	}

	bool same_type = false;

	{
		auto interop = dynamic_pointer_cast<ReelInteropTextAsset>(_main_subtitle);
		auto interop_other = dynamic_pointer_cast<ReelInteropTextAsset>(other->_main_subtitle);
		if (interop && interop_other) {
			same_type = true;
			if (!interop->equals(interop_other, opt, note)) {
				return false;
			}
		}
	}

	{
		auto smpte = dynamic_pointer_cast<ReelSMPTETextAsset>(_main_subtitle);
		auto smpte_other = dynamic_pointer_cast<ReelSMPTETextAsset>(other->_main_subtitle);
		if (smpte && smpte_other) {
			same_type = true;
			if (!smpte->equals(smpte_other, opt, note)) {
				return false;
			}
		}
	}

	if ((_main_subtitle || other->_main_subtitle) && !same_type) {
		return false;
	}

	if ((_main_markers && !other->_main_markers) || (!_main_markers && other->_main_markers)) {
		note (NoteType::ERROR, "Reel: one has markers and the other does not");
		return false;
	}

	if (_main_markers && !_main_markers->equals(other->_main_markers, opt, note)) {
		note (NoteType::ERROR, "Reel: marker assets differ");
		return false;
	}

	if (_closed_captions.size() != other->_closed_captions.size()) {
		return false;
	}

	auto i = _closed_captions.begin();
	auto j = other->_closed_captions.begin();
	while (i != _closed_captions.end()) {
		if (!(*i)->equals(*j, opt, note)) {
			return false;
		}
		++i;
		++j;
	}

	if ((_atmos && !other->_atmos) || (!_atmos && other->_atmos)) {
		note (NoteType::ERROR, "Reel: atmos assets differ");
		return false;
	}

	if (_atmos && !_atmos->equals (other->_atmos, opt, note)) {
		return false;
	}

	return true;
}


bool
Reel::any_encrypted () const
{
	auto ecc = false;
	for (auto i: _closed_captions) {
		if (i->encrypted()) {
			ecc = true;
		}
	}

	return (
		(_main_picture && _main_picture->encrypted()) ||
		(_main_sound && _main_sound->encrypted()) ||
		(_main_subtitle && _main_subtitle->encrypted()) ||
		ecc ||
		(_atmos && _atmos->encrypted())
		);
}


bool
Reel::all_encrypted () const
{
	auto ecc = true;
	for (auto i: _closed_captions) {
		if (!i->encrypted()) {
			ecc = false;
		}
	}

	return (
		(!_main_picture || _main_picture->encrypted()) &&
		(!_main_sound || _main_sound->encrypted()) &&
		(!_main_subtitle || _main_subtitle->encrypted()) &&
		ecc &&
		(!_atmos || _atmos->encrypted())
	       );
}


void
Reel::add (DecryptedKDM const & kdm)
{
	give_kdm_to_assets (kdm);
	/* We have to keep the KDMs that we are given, as they will not be passed to unresolved assets.
	 * After we resolve some assets we will re-call give_kdm_to_assets() with all the KDMs that
	 * we have been given so far.
	 */
	_kdms.push_back (kdm);
}


void
Reel::give_kdm_to_assets (DecryptedKDM const & kdm)
{
	for (auto const& i: kdm.keys()) {
		if (_main_picture && i.id() == _main_picture->key_id() && _main_picture->asset_ref().resolved()) {
			_main_picture->j2k_asset()->set_key(i.key());
		}
		if (_main_sound && i.id() == _main_sound->key_id() && _main_sound->asset_ref().resolved()) {
			_main_sound->asset()->set_key (i.key());
		}
		if (_main_subtitle) {
			auto smpte = dynamic_pointer_cast<ReelSMPTETextAsset>(_main_subtitle);
			if (smpte && i.id() == smpte->key_id() && smpte->asset_ref().resolved()) {
				smpte->smpte_asset()->set_key(i.key());
			}
		}
		for (auto j: _closed_captions) {
			auto smpte = dynamic_pointer_cast<ReelSMPTETextAsset>(j);
			if (smpte && i.id() == smpte->key_id() && smpte->asset_ref().resolved()) {
				smpte->smpte_asset()->set_key(i.key());
			}
		}
		if (_atmos && i.id() == _atmos->key_id() && _atmos->asset_ref().resolved()) {
			_atmos->asset()->set_key (i.key());
		}
	}
}


void
Reel::add (shared_ptr<ReelAsset> asset)
{
	if (auto p = dynamic_pointer_cast<ReelPictureAsset>(asset)) {
		_main_picture = p;
	} else if (auto so = dynamic_pointer_cast<ReelSoundAsset>(asset)) {
		_main_sound = so;
	} else if (auto te = dynamic_pointer_cast<ReelTextAsset>(asset)) {
		switch (te->type()) {
		case TextType::OPEN_SUBTITLE:
			_main_subtitle = te;
			break;
		case TextType::OPEN_CAPTION:
			_main_caption = te;
			break;
		case TextType::CLOSED_SUBTITLE:
			_closed_subtitles.push_back(te);
			break;
		case TextType::CLOSED_CAPTION:
			_closed_captions.push_back(te);
			break;
		}
	} else if (auto m = dynamic_pointer_cast<ReelMarkersAsset>(asset)) {
		_main_markers = m;
	} else if (auto a = dynamic_pointer_cast<ReelAtmosAsset>(asset)) {
		_atmos = a;
	} else {
		DCP_ASSERT(false);
	}
}


vector<shared_ptr<ReelAsset>>
Reel::assets () const
{
	vector<shared_ptr<ReelAsset>> a;
	if (_main_picture) {
		a.push_back (_main_picture);
	}
	if (_main_sound) {
		a.push_back (_main_sound);
	}
	if (_main_subtitle) {
		a.push_back (_main_subtitle);
	}
	if (_main_caption) {
		a.push_back(_main_caption);
	}
	std::copy (_closed_subtitles.begin(), _closed_subtitles.end(), back_inserter(a));
	std::copy (_closed_captions.begin(), _closed_captions.end(), back_inserter(a));
	if (_atmos) {
		a.push_back (_atmos);
	}
	return a;
}


void
Reel::resolve_refs (vector<shared_ptr<Asset>> assets)
{
	if (_main_picture) {
		_main_picture->asset_ref().resolve(assets);
	}

	if (_main_sound) {
		_main_sound->asset_ref().resolve(assets);
	}

	auto resolve_interop_fonts = [&assets](shared_ptr<ReelTextAsset>(asset)) {
		/* Interop subtitle handling is all special cases */
		if (asset->asset_ref().resolved()) {
			if (auto iop = dynamic_pointer_cast<InteropTextAsset>(asset->asset_ref().asset())) {
				iop->resolve_fonts(assets);
			}
		}

	};

	if (_main_subtitle) {
		_main_subtitle->asset_ref().resolve(assets);
		resolve_interop_fonts(_main_subtitle);
	}

	if (_main_caption) {
		_main_caption->asset_ref().resolve(assets);
	}

	for (auto i: _closed_subtitles) {
		i->asset_ref().resolve(assets);
		resolve_interop_fonts(i);
	}

	for (auto i: _closed_captions) {
		i->asset_ref().resolve(assets);
		resolve_interop_fonts(i);
	}

	if (_atmos) {
		_atmos->asset_ref().resolve (assets);
	}

	for (auto const& i: _kdms) {
		give_kdm_to_assets (i);
	}
}


int64_t
Reel::duration () const
{
	if (_main_picture) {
		return _main_picture->actual_duration();
	}

	int64_t d = INT64_MAX;

	if (_main_sound) {
		d = min (d, _main_sound->actual_duration());
	}
	if (_main_subtitle) {
		d = min (d, _main_subtitle->actual_duration());
	}
	if (_main_caption) {
		d = min(d, _main_caption->actual_duration());
	}
	if (_main_markers) {
		d = min (d, _main_markers->actual_duration());
	}
	for (auto i: _closed_subtitles) {
		d = min(d, i->actual_duration());
	}
	for (auto i: _closed_captions) {
		d = min (d, i->actual_duration());
	}
	if (_atmos) {
		d = min (d, _atmos->actual_duration());
	}

	DCP_ASSERT (d < INT64_MAX);

	return d;
}
