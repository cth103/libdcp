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


/** @file  src/reel_encryptable_asset.cc
 *  @brief ReelEncryptableAsset class
 */


#include "reel_encryptable_asset.h"
#include "util.h"
#include "mxf.h"
#include "dcp_assert.h"
#include <libcxml/cxml.h>
#include <libxml++/libxml++.h>


using std::string;
using std::shared_ptr;
using boost::optional;
using namespace dcp;


ReelEncryptableAsset::ReelEncryptableAsset (optional<string> key_id)
	: _key_id (key_id)
{

}


ReelEncryptableAsset::ReelEncryptableAsset (shared_ptr<const cxml::Node> node)
	: _key_id (node->optional_string_child("KeyId"))
{
	if (_key_id) {
		_key_id = remove_urn_uuid (*_key_id);
	}
}


void
ReelEncryptableAsset::write_to_cpl_encryptable (xmlpp::Node* node) const
{
        if (key_id()) {
		auto hash = find_child (node, "Hash");
		node->add_child_before(hash, "KeyId")->add_child_text("urn:uuid:" + key_id().get());
        }
}
