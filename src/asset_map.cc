/*
    Copyright (C) 2012-2022 Carl Hetherington <cth@carlh.net>

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


#include "asset_map.h"
#include "dcp_assert.h"
#include "exceptions.h"
#include "filesystem.h"
#include "raw_convert.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <fmt/format.h>
#include <boost/algorithm/string.hpp>



using std::map;
using std::string;
using std::vector;
using boost::algorithm::starts_with;
using namespace dcp;


static string const assetmap_interop_ns = "http://www.digicine.com/PROTO-ASDCP-AM-20040311#";
static string const assetmap_smpte_ns   = "http://www.smpte-ra.org/schemas/429-9/2007/AM";


AssetMap::AssetMap(boost::filesystem::path file)
	: _file(file)
{
	cxml::Document doc("AssetMap");

	doc.read_file(dcp::filesystem::fix_long_path(file));
	if (doc.namespace_uri() == assetmap_interop_ns) {
		_standard = Standard::INTEROP;
	} else if (doc.namespace_uri() == assetmap_smpte_ns) {
		_standard = Standard::SMPTE;
	} else {
		boost::throw_exception(XMLError("Unrecognised Assetmap namespace " + doc.namespace_uri()));
	}

	_id = remove_urn_uuid(doc.string_child("Id"));
	_annotation_text = doc.optional_string_child("AnnotationText");
        _issue_date = doc.string_child("IssueDate");
        _issuer = doc.string_child("Issuer");
        _creator = doc.string_child("Creator");

	for (auto asset: doc.node_child("AssetList")->node_children("Asset")) {
                _assets.push_back(Asset(asset, _file->parent_path(), _standard));
        }
}


AssetMap::Asset::Asset(cxml::ConstNodePtr node, boost::filesystem::path root, dcp::Standard standard)
        : Object(remove_urn_uuid(node->string_child("Id")))
{
	if (node->node_child("ChunkList")->node_children("Chunk").size() != 1) {
		boost::throw_exception (XMLError ("unsupported asset chunk count"));
	}

	auto path_from_xml = node->node_child("ChunkList")->node_child("Chunk")->string_child("Path");
	if (starts_with(path_from_xml, "file://")) {
		path_from_xml = path_from_xml.substr(7);
	}

	_path = root / path_from_xml;

	switch (standard) {
	case Standard::INTEROP:
		_pkl = static_cast<bool>(node->optional_node_child("PackingList"));
		break;
	case Standard::SMPTE:
	{
		auto pkl_bool = node->optional_string_child("PackingList");
		_pkl = pkl_bool && *pkl_bool == "true";
		break;
	}
	}
}


void
AssetMap::add_asset(string id, boost::filesystem::path path, bool pkl)
{
        _assets.push_back(Asset(id, path, pkl));
}


void
AssetMap::clear_assets()
{
	_assets.clear();
}


map<std::string, boost::filesystem::path>
AssetMap::asset_ids_and_paths() const
{
        auto paths = map<string, boost::filesystem::path>();
        for (auto asset: _assets) {
                paths[asset.id()] = asset.path();
        }
        return paths;
}


vector<boost::filesystem::path>
AssetMap::pkl_paths() const
{
        auto paths = std::vector<boost::filesystem::path>();
        for (auto asset: _assets) {
                if (asset.pkl()) {
                        paths.push_back(asset.path());
                }
        }
        return paths;
}


void
AssetMap::write_xml(boost::filesystem::path file) const
{
	xmlpp::Document doc;
	xmlpp::Element* root;

	switch (_standard) {
	case Standard::INTEROP:
		root = doc.create_root_node("AssetMap", assetmap_interop_ns);
		break;
	case Standard::SMPTE:
		root = doc.create_root_node("AssetMap", assetmap_smpte_ns);
		break;
	default:
		DCP_ASSERT (false);
	}

	cxml::add_text_child(root, "Id", "urn:uuid:" + _id);
	if (_annotation_text) {
		cxml::add_text_child(root, "AnnotationText", *_annotation_text);
	}

	switch (_standard) {
	case Standard::INTEROP:
		cxml::add_text_child(root, "VolumeCount", "1");
		cxml::add_text_child(root, "IssueDate", _issue_date);
		cxml::add_text_child(root, "Issuer", _issuer);
		cxml::add_text_child(root, "Creator", _creator);
		break;
	case Standard::SMPTE:
		cxml::add_text_child(root, "Creator", _creator);
		cxml::add_text_child(root, "VolumeCount", "1");
		cxml::add_text_child(root, "IssueDate", _issue_date);
		cxml::add_text_child(root, "Issuer", _issuer);
		break;
	default:
		DCP_ASSERT (false);
	}

	auto asset_list = cxml::add_child(root, "AssetList");
	for (auto const& asset: _assets) {
		asset.write_xml(asset_list, file.parent_path());
	}

	doc.write_to_file_formatted(dcp::filesystem::fix_long_path(file).string(), "UTF-8");
	_file = file;
}


void
AssetMap::Asset::write_xml(xmlpp::Element* asset_list, boost::filesystem::path dcp_root_directory) const
{
	auto node = cxml::add_child(asset_list, "Asset");
	cxml::add_text_child(node, "Id", "urn:uuid:" + _id);
	if (_pkl) {
		cxml::add_text_child(node, "PackingList", "true");
	}
	auto chunk_list = cxml::add_child(node, "ChunkList");
	auto chunk = cxml::add_child(chunk_list, "Chunk");

	auto relative_path = relative_to_root(filesystem::canonical(dcp_root_directory), filesystem::canonical(_path));
	DCP_ASSERT(relative_path);

	cxml::add_text_child(chunk, "Path", relative_path->generic_string());
	cxml::add_text_child(chunk, "VolumeIndex", "1");
	cxml::add_text_child(chunk, "Offset", "0");
	cxml::add_text_child(chunk, "Length", fmt::to_string(filesystem::file_size(_path)));
}

