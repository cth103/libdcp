/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/reel_file_asset.cc
 *  @brief ReelFileAsset class
 */


#include "asset.h"
#include "reel_file_asset.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS


using std::shared_ptr;
using std::string;
using boost::optional;
using namespace dcp;


ReelFileAsset::ReelFileAsset (shared_ptr<Asset> asset, optional<string> key_id, std::string id, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point)
	: ReelAsset (id, edit_rate, intrinsic_duration, entry_point)
	, _asset_ref (asset)
	, _hash (asset->hash())
	, _key_id (key_id)
{

}


ReelFileAsset::ReelFileAsset (shared_ptr<const cxml::Node> node)
	: ReelAsset (node)
	, _asset_ref (remove_urn_uuid(node->string_child("Id")))
	, _hash (node->optional_string_child("Hash"))
	, _key_id (node->optional_string_child("KeyId"))
{
	if (_key_id) {
		_key_id = remove_urn_uuid (*_key_id);
	}
}


bool
ReelFileAsset::file_asset_equals (shared_ptr<const ReelFileAsset> other, EqualityOptions opt, NoteHandler note) const
{
	if (_hash != other->_hash) {
		if (!opt.reel_hashes_can_differ) {
			note (NoteType::ERROR, "Reel: hashes differ");
			return false;
		} else {
			note (NoteType::NOTE, "Reel: hashes differ");
		}
	}

	if (_asset_ref.resolved() && other->_asset_ref.resolved()) {
		return _asset_ref->equals (other->_asset_ref.asset(), opt, note);
	}

	return true;
}


xmlpp::Node *
ReelFileAsset::write_to_cpl (xmlpp::Node* node, Standard standard) const
{
	auto asset = ReelAsset::write_to_cpl (node, standard);
        if (_key_id) {
		asset->add_child("KeyId")->add_child_text("urn:uuid:" + *_key_id);
        }
	if (_hash) {
		asset->add_child("Hash")->add_child_text(*_hash);
	}
	return asset;
}

