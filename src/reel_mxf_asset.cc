/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

#include "reel_mxf_asset.h"
#include "mxf.h"
#include "dcp_assert.h"
#include <libcxml/cxml.h>
#include <libxml++/libxml++.h>

using boost::shared_ptr;
using namespace dcp;

ReelMXFAsset::ReelMXFAsset ()
	: ReelAsset ()
{

}

ReelMXFAsset::ReelMXFAsset (shared_ptr<MXF> mxf, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point)
	: ReelAsset (mxf, edit_rate, intrinsic_duration, entry_point)
	, _key_id (mxf->key_id ())
{

}

ReelMXFAsset::ReelMXFAsset (shared_ptr<const cxml::Node> node)
	: ReelAsset (node)
	, _key_id (node->optional_string_child ("KeyId").get_value_or (""))
{
	if (_key_id.length() > 9) {
		_key_id = _key_id.substr (9);
	}
}
