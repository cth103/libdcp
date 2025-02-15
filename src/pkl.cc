/*
    Copyright (C) 2018-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/pkl.cc
 *  @brief PKL class
 */


#include "dcp_assert.h"
#include "exceptions.h"
#include "filesystem.h"
#include "pkl.h"
#include "raw_convert.h"
#include "util.h"
#include "warnings.h"
#include "verify.h"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <fmt/format.h>
#include <iostream>


using std::string;
using std::shared_ptr;
using std::make_shared;
using std::vector;
using boost::optional;
using namespace dcp;


static string const pkl_interop_ns = "http://www.digicine.com/PROTO-ASDCP-PKL-20040311#";
static string const pkl_smpte_ns   = "http://www.smpte-ra.org/schemas/429-8/2007/PKL";
/* I don't know why Resolve are using this namespace but apparently they are */
static string const pkl_resolve_smpte_ns = "http://www.smpte-ra.org/schemas/2067-2/2016/PKL";


PKL::PKL(boost::filesystem::path file, vector<dcp::VerificationNote>* notes)
	: _file (file)
{
	cxml::Document pkl ("PackingList");
	pkl.read_file(dcp::filesystem::fix_long_path(file));

	if (pkl.namespace_uri() == pkl_interop_ns) {
		_standard = Standard::INTEROP;
	} else if (pkl.namespace_uri() == pkl_smpte_ns) {
		_standard = Standard::SMPTE;
	} else if (pkl.namespace_uri() == pkl_resolve_smpte_ns) {
		_standard = Standard::SMPTE;
		if (notes) {
			notes->push_back(
				dcp::VerificationNote(
					dcp::VerificationNote::Type::ERROR,
					dcp::VerificationNote::Code::INVALID_PKL_NAMESPACE,
					pkl.namespace_uri(),
					file
					)
				);
		}
	} else {
		boost::throw_exception(XMLError("Unrecognised packing list namespace " + pkl.namespace_uri()));
	}

	_id = remove_urn_uuid (pkl.string_child ("Id"));
	_annotation_text = pkl.optional_string_child ("AnnotationText");
	_issue_date = pkl.string_child ("IssueDate");
	_issuer = pkl.string_child ("Issuer");
	_creator = pkl.string_child ("Creator");
	_group_id = remove_urn_uuid(pkl.optional_string_child("GroupId"));

	for (auto i: pkl.node_child("AssetList")->node_children("Asset")) {
		_assets.push_back(make_shared<Asset>(i));
	}
}


void
PKL::add_asset(std::string id, boost::optional<std::string> annotation_text, std::string hash, int64_t size, std::string type, std::string original_filename)
{
	_assets.push_back(make_shared<Asset>(id, annotation_text, hash, size, type, original_filename));
}


void
PKL::write_xml (boost::filesystem::path file, shared_ptr<const CertificateChain> signer) const
{
	xmlpp::Document doc;
	xmlpp::Element* pkl;
	if (_standard == Standard::INTEROP) {
		pkl = doc.create_root_node("PackingList", pkl_interop_ns);
	} else {
		pkl = doc.create_root_node("PackingList", pkl_smpte_ns);
	}

	cxml::add_text_child(pkl, "Id", "urn:uuid:" + _id);
	if (_annotation_text) {
		cxml::add_text_child(pkl, "AnnotationText", *_annotation_text);
	}
	cxml::add_text_child(pkl, "IssueDate", _issue_date);
	cxml::add_text_child(pkl, "Issuer", _issuer);
	cxml::add_text_child(pkl, "Creator", _creator);
	if (_group_id && _standard == Standard::SMPTE) {
		cxml::add_text_child(pkl, "GroupId", "urn:uuid:" + *_group_id);
	}

	auto asset_list = cxml::add_child(pkl, "AssetList");
	for (auto i: _assets) {
		auto asset = cxml::add_child(asset_list, "Asset");
		cxml::add_text_child(asset, "Id", "urn:uuid:" + i->id());
		if (i->annotation_text()) {
			cxml::add_text_child(asset, "AnnotationText", *i->annotation_text());
		}
		cxml::add_text_child(asset, "Hash", i->hash());
		cxml::add_text_child(asset, "Size", fmt::to_string(i->size()));
		cxml::add_text_child(asset, "Type", i->type());
		if (auto filename = i->original_filename()) {
			cxml::add_text_child(asset, "OriginalFileName", *filename);
		}
	}

	indent (pkl, 0);

	if (signer) {
		signer->sign (pkl, _standard);
	}

	doc.write_to_file_formatted(dcp::filesystem::fix_long_path(file).string(), "UTF-8");
	_file = file;
}


optional<string>
PKL::hash (string id) const
{
	for (auto i: _assets) {
		if (i->id() == id) {
			return i->hash();
		}
	}

	return {};
}


optional<string>
PKL::type (string id) const
{
	for (auto i: _assets) {
		if (i->id() == id) {
			return i->type();
		}
	}

	return {};
}


void
PKL::clear_assets()
{
	_assets.clear();
}
