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


/** @file  src/asset.cc
 *  @brief Asset class
 */


#include "asset.h"
#include "asset_map.h"
#include "compose.hpp"
#include "dcp_assert.h"
#include "equality_options.h"
#include "exceptions.h"
#include "filesystem.h"
#include "pkl.h"
#include "raw_convert.h"
#include "util.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <boost/algorithm/string.hpp>


using std::string;
using boost::function;
using std::shared_ptr;
using boost::optional;
using namespace dcp;


Asset::Asset ()
{

}


Asset::Asset(boost::filesystem::path file)
	: _file (file)
{

}


Asset::Asset(string id, boost::filesystem::path file)
	: Object (id)
	, _file (file)
{

}


void
Asset::add_to_pkl(shared_ptr<PKL> pkl, boost::filesystem::path root) const
{
	DCP_ASSERT (_file);

	auto path = relative_to_root (
		filesystem::canonical(root),
		filesystem::canonical(_file.get())
		);

	if (!path) {
		/* The path of this asset is not within our DCP, so we assume it's an external
		   (referenced) one.
		*/
		return;
	}

	pkl->add_asset(_id, _id, hash(), file_size(_file.get()), pkl_type(pkl->standard()), _file->filename().string());
}


void
Asset::add_to_assetmap(AssetMap& asset_map, boost::filesystem::path root) const
{
	DCP_ASSERT (_file);
	add_file_to_assetmap (asset_map, root, _file.get(), _id);
}


void
Asset::add_file_to_assetmap(AssetMap& asset_map, boost::filesystem::path root, boost::filesystem::path file, string id)
{
	auto path = relative_to_root (
		filesystem::canonical(root),
		filesystem::canonical(file)
		);

	if (!path) {
		/* The path of this asset is not within our DCP, so we assume it's an external
		   (referenced) one.
		*/
		return;
	}

	asset_map.add_asset(id, file, false);
}


string
Asset::hash(function<void (int64_t, int64_t)> progress) const
{
	DCP_ASSERT (_file);

	if (!_hash) {
		_hash = make_digest (_file.get(), progress);
	}

	return _hash.get();
}


bool
Asset::equals(std::shared_ptr<const Asset> other, EqualityOptions const& opt, NoteHandler note) const
{
	if (_hash != other->_hash) {
		if (!opt.asset_hashes_can_differ) {
			note(NoteType::ERROR, "Asset: hashes differ");
			return false;
		} else {
			note(NoteType::NOTE, "Asset: hashes differ");
		}
	}

	return true;
}


void
Asset::set_file(boost::filesystem::path file) const
{
	_file = filesystem::absolute(file);
	_hash = optional<string>();
}


void
Asset::set_file_preserving_hash(boost::filesystem::path file) const
{
	_file = filesystem::absolute(file);
}


void
Asset::rename_file(boost::filesystem::path file)
{
	_file = filesystem::absolute(file);
}


void
Asset::set_hash (string hash)
{
	_hash = hash;
}


void
Asset::unset_hash()
{
	_hash = optional<string>();
}

