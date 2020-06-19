/*
    Copyright (C) 2012-2019 Carl Hetherington <cth@carlh.net>

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

#include "reel_mxf.h"
#include "util.h"
#include "mxf.h"
#include "dcp_assert.h"
#include <libcxml/cxml.h>
#include <libxml++/libxml++.h>

using std::string;
using boost::shared_ptr;
using boost::optional;
using namespace dcp;

ReelMXF::ReelMXF (shared_ptr<Asset> asset, optional<string> key_id)
	: _asset_ref (asset)
	, _key_id (key_id)
	, _hash (asset->hash())
{

}

ReelMXF::ReelMXF (shared_ptr<const cxml::Node> node)
	: _asset_ref (remove_urn_uuid(node->string_child("Id")))
	, _key_id (node->optional_string_child ("KeyId"))
	, _hash (node->optional_string_child ("Hash"))
{
	if (_key_id) {
		_key_id = remove_urn_uuid (*_key_id);
	}
}

bool
ReelMXF::mxf_equals (shared_ptr<const ReelMXF> other, EqualityOptions opt, NoteHandler note) const
{
	if (_hash != other->_hash) {
		if (!opt.reel_hashes_can_differ) {
			note (DCP_ERROR, "Reel: hashes differ");
			return false;
		} else {
			note (DCP_NOTE, "Reel: hashes differ");
		}
	}

	if (_asset_ref.resolved() && other->_asset_ref.resolved()) {
		return _asset_ref->equals (other->_asset_ref.asset(), opt, note);
	}

	return true;
}


void
ReelMXF::write_to_cpl_mxf (xmlpp::Node* node) const
{
        if (key_id ()) {
		xmlpp::Node* hash = find_child (node, "Hash");
		node->add_child_before(hash, "KeyId")->add_child_text("urn:uuid:" + key_id().get());
        }
}
