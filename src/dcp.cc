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


/** @file  src/dcp.cc
 *  @brief DCP class
 */


#include "asset_factory.h"
#include "atmos_asset.h"
#include "certificate_chain.h"
#include "compose.hpp"
#include "cpl.h"
#include "dcp.h"
#include "dcp_assert.h"
#include "decrypted_kdm.h"
#include "decrypted_kdm_key.h"
#include "exceptions.h"
#include "filesystem.h"
#include "font_asset.h"
#include "interop_subtitle_asset.h"
#include "metadata.h"
#include "mono_picture_asset.h"
#include "picture_asset.h"
#include "pkl.h"
#include "raw_convert.h"
#include "reel_asset.h"
#include "reel_subtitle_asset.h"
#include "smpte_subtitle_asset.h"
#include "sound_asset.h"
#include "stereo_picture_asset.h"
#include "util.h"
#include "verify.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <asdcp/AS_DCP.h>
LIBDCP_ENABLE_WARNINGS
#include <xmlsec/xmldsig.h>
#include <xmlsec/app.h>
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <boost/algorithm/string.hpp>
#include <numeric>


using std::cerr;
using std::cout;
using std::dynamic_pointer_cast;
using std::exception;
using std::list;
using std::make_pair;
using std::make_shared;
using std::map;
using std::shared_ptr;
using std::string;
using std::vector;
using boost::algorithm::starts_with;
using boost::optional;
using namespace dcp;


static string const volindex_interop_ns = "http://www.digicine.com/PROTO-ASDCP-VL-20040311#";
static string const volindex_smpte_ns   = "http://www.smpte-ra.org/schemas/429-9/2007/AM";


DCP::DCP (boost::filesystem::path directory)
	: _directory (directory)
{
	if (!filesystem::exists(directory)) {
		filesystem::create_directories(directory);
	}

	_directory = filesystem::canonical(_directory);
}


DCP::DCP(DCP&& other)
	: _directory(std::move(other._directory))
	, _cpls(std::move(other._cpls))
	, _pkls(std::move(other._pkls))
	, _asset_map(std::move(other._asset_map))
	, _new_issuer(std::move(other._new_issuer))
	, _new_creator(std::move(other._new_creator))
	, _new_issue_date(std::move(other._new_issue_date))
	, _new_annotation_text(std::move(other._new_annotation_text))
{

}


DCP&
DCP::operator=(DCP&& other)
{
	_directory = std::move(other._directory);
	_cpls = std::move(other._cpls);
	_pkls = std::move(other._pkls);
	_asset_map = std::move(other._asset_map);
	_new_issuer = std::move(other._new_issuer);
	_new_creator = std::move(other._new_creator);
	_new_issue_date = std::move(other._new_issue_date);
	_new_annotation_text = std::move(other._new_annotation_text);
	return *this;
}


void
DCP::read (vector<dcp::VerificationNote>* notes, bool ignore_incorrect_picture_mxf_type)
{
	/* Read the ASSETMAP and PKL */

	boost::filesystem::path asset_map_path;
	if (filesystem::exists(_directory / "ASSETMAP")) {
		asset_map_path = _directory / "ASSETMAP";
	} else if (filesystem::exists(_directory / "ASSETMAP.xml")) {
		asset_map_path = _directory / "ASSETMAP.xml";
	} else {
		boost::throw_exception(MissingAssetmapError(_directory));
	}

	_asset_map = AssetMap(asset_map_path);
	auto const pkl_paths = _asset_map->pkl_paths();
	auto const standard = _asset_map->standard();

	if (pkl_paths.empty()) {
		boost::throw_exception (XMLError ("No packing lists found in asset map"));
	}

	for (auto i: pkl_paths) {
		_pkls.push_back(make_shared<PKL>(i, notes));
	}

	/* Now we have:
	     paths - map of files in the DCP that are not PKLs; key is ID, value is path.
	     _pkls - PKL objects for each PKL.

	   Read all the assets from the asset map.
	 */

	/* Make a list of non-CPL/PKL assets so that we can resolve the references
	   from the CPLs.
	*/
	vector<shared_ptr<Asset>> other_assets;

	auto ids_and_paths = _asset_map->asset_ids_and_paths();
	for (auto id_and_path: ids_and_paths) {
		auto const id = id_and_path.first;
		auto const path = id_and_path.second;

		if (path == _directory) {
			/* I can't see how this is valid, but it's
			   been seen in the wild with a DCP that
			   claims to come from ClipsterDCI 5.10.0.5.
			*/
			if (notes) {
				notes->push_back ({VerificationNote::Type::WARNING, VerificationNote::Code::EMPTY_ASSET_PATH});
			}
			continue;
		}

		if (!filesystem::exists(path)) {
			if (notes) {
				notes->push_back ({VerificationNote::Type::ERROR, VerificationNote::Code::MISSING_ASSET, path});
			}
			continue;
		}

		/* Find the <Type> for this asset from the PKL that contains the asset */
		optional<string> pkl_type;
		for (auto j: _pkls) {
			pkl_type = j->type(id);
			if (pkl_type) {
				break;
			}
		}

		if (!pkl_type) {
			/* This asset is in the ASSETMAP but not mentioned in any PKL so we don't
			 * need to worry about it.
			 */
			continue;
		}

		auto remove_parameters = [](string const& n) {
			return n.substr(0, n.find(";"));
		};

		/* Remove any optional parameters (after ;) */
		pkl_type = pkl_type->substr(0, pkl_type->find(";"));

		if (
			pkl_type == remove_parameters(CPL::static_pkl_type(standard)) ||
			pkl_type == remove_parameters(InteropSubtitleAsset::static_pkl_type(standard))) {
			auto p = new xmlpp::DomParser;
			try {
				p->parse_file(dcp::filesystem::fix_long_path(path).string());
			} catch (std::exception& e) {
				delete p;
				throw ReadError(String::compose("XML error in %1", path.string()), e.what());
			}

			auto const root = p->get_document()->get_root_node()->get_name();
			delete p;

			if (root == "CompositionPlaylist") {
				auto cpl = make_shared<CPL>(path, notes);
				if (cpl->standard() != standard && notes) {
					notes->push_back ({VerificationNote::Type::ERROR, VerificationNote::Code::MISMATCHED_STANDARD});
				}
				_cpls.push_back (cpl);
			} else if (root == "DCSubtitle") {
				if (standard == Standard::SMPTE && notes) {
					notes->push_back (VerificationNote(VerificationNote::Type::ERROR, VerificationNote::Code::MISMATCHED_STANDARD));
				}
				other_assets.push_back (make_shared<InteropSubtitleAsset>(path));
			}
		} else if (
			*pkl_type == remove_parameters(PictureAsset::static_pkl_type(standard)) ||
			*pkl_type == remove_parameters(SoundAsset::static_pkl_type(standard)) ||
			*pkl_type == remove_parameters(AtmosAsset::static_pkl_type(standard)) ||
			*pkl_type == remove_parameters(SMPTESubtitleAsset::static_pkl_type(standard))
			) {

			bool found_threed_marked_as_twod = false;
			auto asset = asset_factory(path, ignore_incorrect_picture_mxf_type, &found_threed_marked_as_twod);
			if (asset->id() != id) {
				notes->push_back(VerificationNote(VerificationNote::Type::ERROR, VerificationNote::Code::MISMATCHED_ASSET_MAP_ID).set_id(id).set_other_id(asset->id()));
			}
			other_assets.push_back(asset);
			if (found_threed_marked_as_twod && notes) {
				notes->push_back ({VerificationNote::Type::WARNING, VerificationNote::Code::THREED_ASSET_MARKED_AS_TWOD, path});
			}
		} else if (*pkl_type == remove_parameters(FontAsset::static_pkl_type(standard))) {
			other_assets.push_back(make_shared<FontAsset>(id, path));
		} else if (*pkl_type == "image/png") {
			/* It's an Interop PNG subtitle; let it go */
		} else {
			throw ReadError (String::compose("Unknown asset type %1 in PKL", *pkl_type));
		}
	}

	/* Set hashes for assets where we have an idea of what the hash should be in either a CPL or PKL.
	 * This means that when the hash is later read from these objects the result will be the one that
	 * it should be, rather the one that it currently is.  This should prevent errors being concealed
	 * when an asset is corrupted - the hash from the CPL/PKL will disagree with the actual hash of the
	 * file, revealing the problem.
	 */

	auto hash_from_pkl = [this](string id) -> optional<string> {
		for (auto pkl: _pkls) {
			if (auto pkl_hash = pkl->hash(id)) {
				return pkl_hash;
			}
		}

		return {};
	};

	auto hash_from_cpl_or_pkl = [this, &hash_from_pkl](string id) -> optional<string> {
		for (auto cpl: cpls()) {
			for (auto reel_file_asset: cpl->reel_file_assets()) {
				if (reel_file_asset->asset_ref().id() == id && reel_file_asset->hash()) {
					return reel_file_asset->hash();
				}
			}
		}

		return hash_from_pkl(id);
	};

	for (auto asset: other_assets) {
		if (auto hash = hash_from_cpl_or_pkl(asset->id())) {
			asset->set_hash(*hash);
		}
	}

	for (auto cpl: cpls()) {
		if (auto hash = hash_from_pkl(cpl->id())) {
			cpl->set_hash(*hash);
		}
	}

	/* Resolve references */
	resolve_refs (other_assets);

	/* While we've got the ASSETMAP lets look and see if this DCP refers to things that are not in its ASSETMAP */
	if (notes) {
		for (auto i: cpls()) {
			for (auto j: i->reel_file_assets()) {
				if (!j->asset_ref().resolved() && ids_and_paths.find(j->asset_ref().id()) == ids_and_paths.end()) {
					notes->push_back (VerificationNote(VerificationNote::Type::WARNING, VerificationNote::Code::EXTERNAL_ASSET, j->asset_ref().id()));
				}
			}
		}
	}
}


void
DCP::resolve_refs (vector<shared_ptr<Asset>> assets)
{
	for (auto i: cpls()) {
		i->resolve_refs (assets);
	}
}


bool
DCP::equals(DCP const & other, EqualityOptions const& opt, NoteHandler note) const
{
	auto a = cpls ();
	auto b = other.cpls ();

	if (a.size() != b.size()) {
		note (NoteType::ERROR, String::compose ("CPL counts differ: %1 vs %2", a.size(), b.size()));
		return false;
	}

	bool r = true;

	for (auto i: a) {
		auto j = b.begin();
		while (j != b.end() && !(*j)->equals (i, opt, note)) {
			++j;
		}

		if (j == b.end ()) {
			r = false;
		}
	}

	return r;
}


void
DCP::add (shared_ptr<CPL> cpl)
{
	_cpls.push_back (cpl);
}


bool
DCP::any_encrypted () const
{
	for (auto i: cpls()) {
		if (i->any_encrypted()) {
			return true;
		}
	}

	return false;
}


bool
DCP::all_encrypted () const
{
	for (auto i: cpls()) {
		if (!i->all_encrypted()) {
			return false;
		}
	}

	return true;
}


void
DCP::add (DecryptedKDM const & kdm)
{
	auto keys = kdm.keys();
	for (auto cpl: cpls()) {
		if (std::any_of(keys.begin(), keys.end(), [cpl](DecryptedKDMKey const& key) { return key.cpl_id() == cpl->id(); })) {
			cpl->add (kdm);
		}
	}
}


/** Write the VOLINDEX file.
 *  @param standard DCP standard to use (INTEROP or SMPTE)
 */
void
DCP::write_volindex (Standard standard) const
{
	auto p = _directory;
	switch (standard) {
	case Standard::INTEROP:
		p /= "VOLINDEX";
		break;
	case Standard::SMPTE:
		p /= "VOLINDEX.xml";
		break;
	default:
		DCP_ASSERT (false);
	}

	xmlpp::Document doc;
	xmlpp::Element* root;

	switch (standard) {
	case Standard::INTEROP:
		root = doc.create_root_node ("VolumeIndex", volindex_interop_ns);
		break;
	case Standard::SMPTE:
		root = doc.create_root_node ("VolumeIndex", volindex_smpte_ns);
		break;
	default:
		DCP_ASSERT (false);
	}

	root->add_child("Index")->add_child_text ("1");
	doc.write_to_file_formatted(dcp::filesystem::fix_long_path(p).string(), "UTF-8");
}


void
DCP::write_xml(shared_ptr<const CertificateChain> signer, bool include_mca_subdescriptors, NameFormat name_format)
{
	if (_cpls.empty()) {
		throw MiscError ("Cannot write DCP with no CPLs.");
	}

	auto standard = std::accumulate (
		std::next(_cpls.begin()), _cpls.end(), _cpls[0]->standard(),
		[](Standard s, shared_ptr<CPL> c) {
			if (s != c->standard()) {
				throw MiscError ("Cannot make DCP with mixed Interop and SMPTE CPLs.");
			}
			return s;
		}
		);

	for (auto i: cpls()) {
		NameFormat::Map values;
		values['t'] = "cpl";
		i->write_xml(_directory / (name_format.get(values, "_" + i->id() + ".xml")), signer, include_mca_subdescriptors);
	}

	if (_pkls.empty()) {
		_pkls.push_back(
			make_shared<PKL>(
				standard,
				_new_annotation_text.get_value_or(String::compose("Created by libdcp %1", dcp::version)),
				_new_issue_date.get_value_or(LocalTime().as_string()),
				_new_issuer.get_value_or(String::compose("libdcp %1", dcp::version)),
				_new_creator.get_value_or(String::compose("libdcp %1", dcp::version))
				)
			);
	}

	auto pkl = _pkls.front();

	/* The assets may have changed since we read the PKL, so re-add them */
	pkl->clear_assets();
	for (auto asset: assets()) {
		asset->add_to_pkl(pkl, _directory);
	}

	NameFormat::Map values;
	values['t'] = "pkl";
	auto pkl_path = _directory / name_format.get(values, "_" + pkl->id() + ".xml");
	pkl->write_xml (pkl_path, signer);

	if (!_asset_map) {
		_asset_map = AssetMap(
			standard,
			_new_annotation_text.get_value_or(String::compose("Created by libdcp %1", dcp::version)),
			_new_issue_date.get_value_or(LocalTime().as_string()),
			_new_issuer.get_value_or(String::compose("libdcp %1", dcp::version)),
			_new_creator.get_value_or(String::compose("libdcp %1", dcp::version))
			);
	}

	/* The assets may have changed since we read the asset map, so re-add them */
	_asset_map->clear_assets();
	_asset_map->add_asset(pkl->id(), pkl_path, true);
	for (auto asset: assets()) {
		asset->add_to_assetmap(*_asset_map, _directory);
	}

	_asset_map->write_xml(
		_directory / (standard == Standard::INTEROP ? "ASSETMAP" : "ASSETMAP.xml")
		);

	write_volindex (standard);
}


vector<shared_ptr<CPL>>
DCP::cpls () const
{
	return _cpls;
}


vector<shared_ptr<Asset>>
DCP::assets (bool ignore_unresolved) const
{
	vector<shared_ptr<Asset>> assets;
	for (auto i: cpls()) {
		assets.push_back (i);
		for (auto j: i->reel_file_assets()) {
			if (ignore_unresolved && !j->asset_ref().resolved()) {
				continue;
			}

			auto const id = j->asset_ref().id();
			if (std::find_if(assets.begin(), assets.end(), [id](shared_ptr<Asset> asset) { return asset->id() == id; }) == assets.end()) {
				auto o = j->asset_ref().asset();
				assets.push_back (o);
				/* More Interop special-casing */
				auto sub = dynamic_pointer_cast<InteropSubtitleAsset>(o);
				if (sub) {
					add_to_container(assets, sub->font_assets());
				}
			}
		}
	}

	return assets;
}


/** Given a list of files that make up 1 or more DCPs, return the DCP directories */
vector<boost::filesystem::path>
DCP::directories_from_files (vector<boost::filesystem::path> files)
{
	vector<boost::filesystem::path> d;
	for (auto i: files) {
		if (i.filename() == "ASSETMAP" || i.filename() == "ASSETMAP.xml") {
			d.push_back (i.parent_path ());
		}
	}
	return d;
}


void
DCP::set_issuer(string issuer)
{
	for (auto pkl: _pkls) {
		pkl->set_issuer(issuer);
	}
	if (_asset_map) {
		_asset_map->set_issuer(issuer);
	}
	_new_issuer = issuer;
}


void
DCP::set_creator(string creator)
{
	for (auto pkl: _pkls) {
		pkl->set_creator(creator);
	}
	if (_asset_map) {
		_asset_map->set_creator(creator);
	}
	_new_creator = creator;
}


void
DCP::set_issue_date(string issue_date)
{
	for (auto pkl: _pkls) {
		pkl->set_issue_date(issue_date);
	}
	if (_asset_map) {
		_asset_map->set_issue_date(issue_date);
	}
	_new_issue_date = issue_date;
}


void
DCP::set_annotation_text(string annotation_text)
{
	for (auto pkl: _pkls) {
		pkl->set_annotation_text(annotation_text);
	}
	if (_asset_map) {
		_asset_map->set_annotation_text(annotation_text);
	}
	_new_annotation_text = annotation_text;
}

