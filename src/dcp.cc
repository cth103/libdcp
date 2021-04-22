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
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <numeric>


using std::string;
using std::list;
using std::vector;
using std::cout;
using std::make_pair;
using std::map;
using std::cerr;
using std::make_shared;
using std::exception;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using boost::optional;
using boost::algorithm::starts_with;
using namespace dcp;


static string const assetmap_interop_ns = "http://www.digicine.com/PROTO-ASDCP-AM-20040311#";
static string const assetmap_smpte_ns   = "http://www.smpte-ra.org/schemas/429-9/2007/AM";
static string const volindex_interop_ns = "http://www.digicine.com/PROTO-ASDCP-VL-20040311#";
static string const volindex_smpte_ns   = "http://www.smpte-ra.org/schemas/429-9/2007/AM";


DCP::DCP (boost::filesystem::path directory)
	: _directory (directory)
{
	if (!boost::filesystem::exists (directory)) {
		boost::filesystem::create_directories (directory);
	}

	_directory = boost::filesystem::canonical (_directory);
}


void
DCP::read (vector<dcp::VerificationNote>* notes, bool ignore_incorrect_picture_mxf_type)
{
	/* Read the ASSETMAP and PKL */

	if (boost::filesystem::exists (_directory / "ASSETMAP")) {
		_asset_map = _directory / "ASSETMAP";
	} else if (boost::filesystem::exists (_directory / "ASSETMAP.xml")) {
		_asset_map = _directory / "ASSETMAP.xml";
	} else {
		boost::throw_exception (MissingAssetmapError(_directory));
	}

	cxml::Document asset_map ("AssetMap");

	asset_map.read_file (_asset_map.get());
	if (asset_map.namespace_uri() == assetmap_interop_ns) {
		_standard = Standard::INTEROP;
	} else if (asset_map.namespace_uri() == assetmap_smpte_ns) {
		_standard = Standard::SMPTE;
	} else {
		boost::throw_exception (XMLError ("Unrecognised Assetmap namespace " + asset_map.namespace_uri()));
	}

	auto asset_nodes = asset_map.node_child("AssetList")->node_children ("Asset");
	map<string, boost::filesystem::path> paths;
	vector<boost::filesystem::path> pkl_paths;
	for (auto i: asset_nodes) {
		if (i->node_child("ChunkList")->node_children("Chunk").size() != 1) {
			boost::throw_exception (XMLError ("unsupported asset chunk count"));
		}
		auto p = i->node_child("ChunkList")->node_child("Chunk")->string_child ("Path");
		if (starts_with (p, "file://")) {
			p = p.substr (7);
		}
		switch (*_standard) {
		case Standard::INTEROP:
			if (i->optional_node_child("PackingList")) {
				pkl_paths.push_back (p);
			} else {
				paths.insert (make_pair(remove_urn_uuid(i->string_child("Id")), p));
			}
			break;
		case Standard::SMPTE:
		{
			auto pkl_bool = i->optional_string_child("PackingList");
			if (pkl_bool && *pkl_bool == "true") {
				pkl_paths.push_back (p);
			} else {
				paths.insert (make_pair(remove_urn_uuid(i->string_child("Id")), p));
			}
			break;
		}
		}
	}

	if (pkl_paths.empty()) {
		boost::throw_exception (XMLError ("No packing lists found in asset map"));
	}

	for (auto i: pkl_paths) {
		_pkls.push_back (make_shared<PKL>(_directory / i));
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

	for (auto i: paths) {
		auto path = _directory / i.second;

		if (i.second.empty()) {
			/* I can't see how this is valid, but it's
			   been seen in the wild with a DCP that
			   claims to come from ClipsterDCI 5.10.0.5.
			*/
			if (notes) {
				notes->push_back ({VerificationNote::Type::WARNING, VerificationNote::Code::EMPTY_ASSET_PATH});
			}
			continue;
		}

		if (!boost::filesystem::exists(path)) {
			if (notes) {
				notes->push_back ({VerificationNote::Type::ERROR, VerificationNote::Code::MISSING_ASSET, path});
			}
			continue;
		}

		/* Find the <Type> for this asset from the PKL that contains the asset */
		optional<string> pkl_type;
		for (auto j: _pkls) {
			pkl_type = j->type(i.first);
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
			pkl_type == remove_parameters(CPL::static_pkl_type(*_standard)) ||
			pkl_type == remove_parameters(InteropSubtitleAsset::static_pkl_type(*_standard))) {
			auto p = new xmlpp::DomParser;
			try {
				p->parse_file (path.string());
			} catch (std::exception& e) {
				delete p;
				throw ReadError(String::compose("XML error in %1", path.string()), e.what());
			}

			auto const root = p->get_document()->get_root_node()->get_name();
			delete p;

			if (root == "CompositionPlaylist") {
				auto cpl = make_shared<CPL>(path);
				if (_standard && cpl->standard() != _standard.get() && notes) {
					notes->push_back ({VerificationNote::Type::ERROR, VerificationNote::Code::MISMATCHED_STANDARD});
				}
				_cpls.push_back (cpl);
			} else if (root == "DCSubtitle") {
				if (_standard && _standard.get() == Standard::SMPTE && notes) {
					notes->push_back (VerificationNote(VerificationNote::Type::ERROR, VerificationNote::Code::MISMATCHED_STANDARD));
				}
				other_assets.push_back (make_shared<InteropSubtitleAsset>(path));
			}
		} else if (
			*pkl_type == remove_parameters(PictureAsset::static_pkl_type(*_standard)) ||
			*pkl_type == remove_parameters(SoundAsset::static_pkl_type(*_standard)) ||
			*pkl_type == remove_parameters(AtmosAsset::static_pkl_type(*_standard)) ||
			*pkl_type == remove_parameters(SMPTESubtitleAsset::static_pkl_type(*_standard))
			) {

			bool found_threed_marked_as_twod = false;
			other_assets.push_back (asset_factory(path, ignore_incorrect_picture_mxf_type, &found_threed_marked_as_twod));
			if (found_threed_marked_as_twod && notes) {
				notes->push_back ({VerificationNote::Type::WARNING, VerificationNote::Code::THREED_ASSET_MARKED_AS_TWOD, path});
			}
		} else if (*pkl_type == remove_parameters(FontAsset::static_pkl_type(*_standard))) {
			other_assets.push_back (make_shared<FontAsset>(i.first, path));
		} else if (*pkl_type == "image/png") {
			/* It's an Interop PNG subtitle; let it go */
		} else {
			throw ReadError (String::compose("Unknown asset type %1 in PKL", *pkl_type));
		}
	}

	resolve_refs (other_assets);

	/* While we've got the ASSETMAP lets look and see if this DCP refers to things that are not in its ASSETMAP */
	if (notes) {
		for (auto i: cpls()) {
			for (auto j: i->reel_file_assets()) {
				if (!j->asset_ref().resolved() && paths.find(j->asset_ref().id()) == paths.end()) {
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
DCP::equals (DCP const & other, EqualityOptions opt, NoteHandler note) const
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
	auto keys = kdm.keys ();

	for (auto i: cpls()) {
		for (auto const& j: kdm.keys()) {
			if (j.cpl_id() == i->id()) {
				i->add (kdm);
			}
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
	doc.write_to_file_formatted (p.string (), "UTF-8");
}


void
DCP::write_assetmap (
	Standard standard, string pkl_uuid, boost::filesystem::path pkl_path,
	string issuer, string creator, string issue_date, string annotation_text
	) const
{
	auto p = _directory;

	switch (standard) {
	case Standard::INTEROP:
		p /= "ASSETMAP";
		break;
	case Standard::SMPTE:
		p /= "ASSETMAP.xml";
		break;
	default:
		DCP_ASSERT (false);
	}

	xmlpp::Document doc;
	xmlpp::Element* root;

	switch (standard) {
	case Standard::INTEROP:
		root = doc.create_root_node ("AssetMap", assetmap_interop_ns);
		break;
	case Standard::SMPTE:
		root = doc.create_root_node ("AssetMap", assetmap_smpte_ns);
		break;
	default:
		DCP_ASSERT (false);
	}

	root->add_child("Id")->add_child_text ("urn:uuid:" + make_uuid());
	root->add_child("AnnotationText")->add_child_text (annotation_text);

	switch (standard) {
	case Standard::INTEROP:
		root->add_child("VolumeCount")->add_child_text ("1");
		root->add_child("IssueDate")->add_child_text (issue_date);
		root->add_child("Issuer")->add_child_text (issuer);
		root->add_child("Creator")->add_child_text (creator);
		break;
	case Standard::SMPTE:
		root->add_child("Creator")->add_child_text (creator);
		root->add_child("VolumeCount")->add_child_text ("1");
		root->add_child("IssueDate")->add_child_text (issue_date);
		root->add_child("Issuer")->add_child_text (issuer);
		break;
	default:
		DCP_ASSERT (false);
	}

	auto asset_list = root->add_child ("AssetList");

	auto asset = asset_list->add_child ("Asset");
	asset->add_child("Id")->add_child_text ("urn:uuid:" + pkl_uuid);
	asset->add_child("PackingList")->add_child_text ("true");
	auto chunk_list = asset->add_child ("ChunkList");
	auto chunk = chunk_list->add_child ("Chunk");
	chunk->add_child("Path")->add_child_text (pkl_path.filename().string());
	chunk->add_child("VolumeIndex")->add_child_text ("1");
	chunk->add_child("Offset")->add_child_text ("0");
	chunk->add_child("Length")->add_child_text (raw_convert<string> (boost::filesystem::file_size (pkl_path)));

	for (auto i: assets()) {
		i->write_to_assetmap (asset_list, _directory);
	}

	doc.write_to_file_formatted (p.string (), "UTF-8");
	_asset_map = p;
}


void
DCP::write_xml (
	string issuer,
	string creator,
	string issue_date,
	string annotation_text,
	shared_ptr<const CertificateChain> signer,
	NameFormat name_format
	)
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
		i->write_xml (_directory / (name_format.get(values, "_" + i->id() + ".xml")), signer);
	}

	shared_ptr<PKL> pkl;

	if (_pkls.empty()) {
		pkl = make_shared<PKL>(standard, annotation_text, issue_date, issuer, creator);
		_pkls.push_back (pkl);
		for (auto i: assets()) {
			i->add_to_pkl (pkl, _directory);
		}
        } else {
		pkl = _pkls.front ();
	}

	NameFormat::Map values;
	values['t'] = "pkl";
	auto pkl_path = _directory / name_format.get(values, "_" + pkl->id() + ".xml");
	pkl->write (pkl_path, signer);

	write_volindex (standard);
	write_assetmap (standard, pkl->id(), pkl_path, issuer, creator, issue_date, annotation_text);
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
			auto already_got = false;
			for (auto k: assets) {
				if (k->id() == id) {
					already_got = true;
				}
			}

			if (!already_got) {
				auto o = j->asset_ref().asset();
				assets.push_back (o);
				/* More Interop special-casing */
				auto sub = dynamic_pointer_cast<InteropSubtitleAsset>(o);
				if (sub) {
					sub->add_font_assets (assets);
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
