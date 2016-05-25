/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

ReelMXF::ReelMXF (optional<string> key_id)
	: _key_id (key_id)
{

}

ReelMXF::ReelMXF (shared_ptr<const cxml::Node> node)
	: _key_id (node->optional_string_child ("KeyId"))
{
	if (_key_id) {
		_key_id = remove_urn_uuid (*_key_id);
	}
}
