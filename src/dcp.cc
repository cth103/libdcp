/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

/** @file  src/dcp.cc
 *  @brief DCP class.
 */

#include "raw_convert.h"
#include "dcp.h"
#include "sound_mxf.h"
#include "picture_mxf.h"
#include "subtitle_content.h"
#include "mono_picture_mxf.h"
#include "stereo_picture_mxf.h"
#include "util.h"
#include "metadata.h"
#include "exceptions.h"
#include "reel.h"
#include "cpl.h"
#include "signer.h"
#include "compose.hpp"
#include "AS_DCP.h"
#include "decrypted_kdm.h"
#include "decrypted_kdm_key.h"
#include <xmlsec/xmldsig.h>
#include <xmlsec/app.h>
#include <libxml++/libxml++.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <iostream>

using std::string;
using std::list;
using std::stringstream;
using std::ostream;
using std::make_pair;
using std::map;
using std::cout;
using std::exception;
using boost::shared_ptr;
using boost::algorithm::starts_with;
using namespace dcp;

DCP::DCP (boost::filesystem::path directory)
	: _directory (directory)
{
	if (!boost::filesystem::exists (directory)) {
		boost::filesystem::create_directories (directory);
	}
	
	_directory = boost::filesystem::canonical (_directory);
}

template<class T> void
survivable_error (bool keep_going, dcp::DCP::ReadErrors* errors, T const & e)
{
	if (keep_going) {
		if (errors) {
			errors->push_back (shared_ptr<T> (new T (e)));
		}
	} else {
		throw e;
	}
}

void
DCP::read (bool keep_going, ReadErrors* errors)
{
	/* Read the ASSETMAP */
	
	boost::filesystem::path asset_map_file;
	if (boost::filesystem::exists (_directory / "ASSETMAP")) {
		asset_map_file = _directory / "ASSETMAP";
	} else if (boost::filesystem::exists (_directory / "ASSETMAP.xml")) {
		asset_map_file = _directory / "ASSETMAP.xml";
	} else {
		boost::throw_exception (DCPReadError (String::compose ("could not find AssetMap file `%1'", asset_map_file.string())));
	}

	cxml::Document asset_map ("AssetMap");
	asset_map.read_file (asset_map_file);
	list<shared_ptr<cxml::Node> > asset_nodes = asset_map.node_child("AssetList")->node_children ("Asset");
	map<string, boost::filesystem::path> paths;
	for (list<shared_ptr<cxml::Node> >::const_iterator i = asset_nodes.begin(); i != asset_nodes.end(); ++i) {
		if ((*i)->node_child("ChunkList")->node_children("Chunk").size() != 1) {
			boost::throw_exception (XMLError ("unsupported asset chunk count"));
		}
		string p = (*i)->node_child("ChunkList")->node_child("Chunk")->string_child ("Path");
		if (starts_with (p, "file://")) {
			p = p.substr (7);
		}
		paths.insert (make_pair ((*i)->string_child ("Id"), p));
	}

	/* Read all the assets from the asset map */
	for (map<string, boost::filesystem::path>::const_iterator i = paths.begin(); i != paths.end(); ++i) {
		boost::filesystem::path path = _directory / i->second;

		if (!boost::filesystem::exists (path)) {
			survivable_error (keep_going, errors, MissingAssetError (path));
			continue;
		}
		
		if (boost::algorithm::ends_with (path.string(), ".xml")) {
			xmlpp::DomParser* p = new xmlpp::DomParser;
			try {
				p->parse_file (path.string());
			} catch (std::exception& e) {
				delete p;
				continue;
			}
			
			string const root = p->get_document()->get_root_node()->get_name ();
			delete p;
			
			if (root == "CompositionPlaylist") {
				_assets.push_back (shared_ptr<CPL> (new CPL (path)));
			} else if (root == "DCSubtitle") {
				_assets.push_back (shared_ptr<SubtitleContent> (new SubtitleContent (path, false)));
			}
		} else if (boost::algorithm::ends_with (path.string(), ".mxf")) {
			ASDCP::EssenceType_t type;
			if (ASDCP::EssenceType (path.string().c_str(), type) != ASDCP::RESULT_OK) {
				throw DCPReadError ("Could not find essence type");
			}
			switch (type) {
				case ASDCP::ESS_UNKNOWN:
				case ASDCP::ESS_MPEG2_VES:
					throw DCPReadError ("MPEG2 video essences are not supported");
				case ASDCP::ESS_JPEG_2000:
					_assets.push_back (shared_ptr<MonoPictureMXF> (new MonoPictureMXF (path)));
					break;
				case ASDCP::ESS_PCM_24b_48k:
				case ASDCP::ESS_PCM_24b_96k:
					_assets.push_back (shared_ptr<SoundMXF> (new SoundMXF (path)));
					break;
				case ASDCP::ESS_JPEG_2000_S:
					_assets.push_back (shared_ptr<StereoPictureMXF> (new StereoPictureMXF (path)));
					break;
				case ASDCP::ESS_TIMED_TEXT:
					_assets.push_back (shared_ptr<SubtitleContent> (new SubtitleContent (path, true)));
					break;
				default:
					throw DCPReadError ("Unknown MXF essence type");
				}
		}
	}

	list<shared_ptr<CPL> > cpl = cpls ();
	for (list<shared_ptr<CPL> >::const_iterator i = cpl.begin(); i != cpl.end(); ++i) {
		(*i)->resolve_refs (list_of_type<Asset, Object> (assets ()));
	}
}

bool
DCP::equals (DCP const & other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (_assets.size() != other._assets.size()) {
		note (DCP_ERROR, String::compose ("Asset counts differ: %1 vs %2", _assets.size(), other._assets.size()));
		return false;
	}

	list<shared_ptr<Asset> >::const_iterator a = _assets.begin ();
	list<shared_ptr<Asset> >::const_iterator b = other._assets.begin ();

	while (a != _assets.end ()) {
		if (!(*a)->equals (*b, opt, note)) {
			return false;
		}
		++a;
		++b;
	}

	return true;
}

void
DCP::add (boost::shared_ptr<Asset> asset)
{
	_assets.push_back (asset);
}

bool
DCP::encrypted () const
{
	list<shared_ptr<CPL> > cpl = cpls ();
	for (list<shared_ptr<CPL> >::const_iterator i = cpl.begin(); i != cpl.end(); ++i) {
		if ((*i)->encrypted ()) {
			return true;
		}
	}

	return false;
}

void
DCP::add (DecryptedKDM const & kdm)
{
	list<DecryptedKDMKey> keys = kdm.keys ();
	list<shared_ptr<CPL> > cpl = cpls ();
	
	for (list<shared_ptr<CPL> >::iterator i = cpl.begin(); i != cpl.end(); ++i) {
		for (list<DecryptedKDMKey>::iterator j = keys.begin(); j != keys.end(); ++j) {
			if (j->cpl_id() == (*i)->id()) {
				(*i)->add (kdm);
			}				
		}
	}
}

boost::filesystem::path
DCP::write_pkl (Standard standard, string pkl_uuid, XMLMetadata metadata, shared_ptr<const Signer> signer) const
{
	boost::filesystem::path p = _directory;
	stringstream s;
	s << pkl_uuid << "_pkl.xml";
	p /= s.str();

	xmlpp::Document doc;
	xmlpp::Element* pkl;
	if (standard == INTEROP) {
		pkl = doc.create_root_node("PackingList", "http://www.digicine.com/PROTO-ASDCP-PKL-20040311#");
	} else {
		pkl = doc.create_root_node("PackingList", "http://www.smpte-ra.org/schemas/429-8/2007/PKL");
	}
	
	if (signer) {
		pkl->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "dsig");
	}

	pkl->add_child("Id")->add_child_text ("urn:uuid:" + pkl_uuid);

	/* XXX: this is a bit of a hack */
	assert (cpls().size() > 0);
	pkl->add_child("AnnotationText")->add_child_text (cpls().front()->annotation_text ());
	
	pkl->add_child("IssueDate")->add_child_text (metadata.issue_date);
	pkl->add_child("Issuer")->add_child_text (metadata.issuer);
	pkl->add_child("Creator")->add_child_text (metadata.creator);

	xmlpp::Element* asset_list = pkl->add_child("AssetList");
	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		(*i)->write_to_pkl (asset_list, standard);
	}

	if (signer) {
		signer->sign (pkl, standard);
	}
		
	doc.write_to_file (p.string (), "UTF-8");
	return p.string ();
}

/** Write the VOLINDEX file.
 *  @param standard DCP standard to use (INTEROP or SMPTE)
 */
void
DCP::write_volindex (Standard standard) const
{
	boost::filesystem::path p = _directory;
	switch (standard) {
	case INTEROP:
		p /= "VOLINDEX";
		break;
	case SMPTE:
		p /= "VOLINDEX.xml";
		break;
	default:
		assert (false);
	}

	xmlpp::Document doc;
	xmlpp::Element* root;

	switch (standard) {
	case INTEROP:
		root = doc.create_root_node ("VolumeIndex", "http://www.digicine.com/PROTO-ASDCP-AM-20040311#");
		break;
	case SMPTE:
		root = doc.create_root_node ("VolumeIndex", "http://www.smpte-ra.org/schemas/429-9/2007/AM");
		break;
	default:
		assert (false);
	}
	
	root->add_child("Index")->add_child_text ("1");
	doc.write_to_file (p.string (), "UTF-8");
}

void
DCP::write_assetmap (Standard standard, string pkl_uuid, int pkl_length, XMLMetadata metadata) const
{
	boost::filesystem::path p = _directory;
	
	switch (standard) {
	case INTEROP:
		p /= "ASSETMAP";
		break;
	case SMPTE:
		p /= "ASSETMAP.xml";
		break;
	default:
		assert (false);
	}

	xmlpp::Document doc;
	xmlpp::Element* root;

	switch (standard) {
	case INTEROP:
		root = doc.create_root_node ("AssetMap", "http://www.digicine.com/PROTO-ASDCP-AM-20040311#");
		break;
	case SMPTE:
		root = doc.create_root_node ("AssetMap", "http://www.smpte-ra.org/schemas/429-9/2007/AM");
		break;
	default:
		assert (false);
	}

	root->add_child("Id")->add_child_text ("urn:uuid:" + make_uuid());
	root->add_child("AnnotationText")->add_child_text ("Created by " + metadata.creator);

	switch (standard) {
	case INTEROP:
		root->add_child("VolumeCount")->add_child_text ("1");
		root->add_child("IssueDate")->add_child_text (metadata.issue_date);
		root->add_child("Issuer")->add_child_text (metadata.issuer);
		root->add_child("Creator")->add_child_text (metadata.creator);
		break;
	case SMPTE:
		root->add_child("Creator")->add_child_text (metadata.creator);
		root->add_child("VolumeCount")->add_child_text ("1");
		root->add_child("IssueDate")->add_child_text (metadata.issue_date);
		root->add_child("Issuer")->add_child_text (metadata.issuer);
		break;
	default:
		assert (false);
	}
		
	xmlpp::Node* asset_list = root->add_child ("AssetList");

	xmlpp::Node* asset = asset_list->add_child ("Asset");
	asset->add_child("Id")->add_child_text ("urn:uuid:" + pkl_uuid);
	asset->add_child("PackingList")->add_child_text ("true");
	xmlpp::Node* chunk_list = asset->add_child ("ChunkList");
	xmlpp::Node* chunk = chunk_list->add_child ("Chunk");
	chunk->add_child("Path")->add_child_text (pkl_uuid + "_pkl.xml");
	chunk->add_child("VolumeIndex")->add_child_text ("1");
	chunk->add_child("Offset")->add_child_text ("0");
	chunk->add_child("Length")->add_child_text (raw_convert<string> (pkl_length));
	
	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		(*i)->write_to_assetmap (asset_list, _directory);
	}

	/* This must not be the _formatted version otherwise signature digests will be wrong */
	doc.write_to_file (p.string (), "UTF-8");
}

/** Write all the XML files for this DCP.
 *  @param standand INTEROP or SMPTE.
 *  @param metadata Metadata to use for PKL and asset map files.
 *  @param signer Signer to use, or 0.
 */
void
DCP::write_xml (
	Standard standard,
	XMLMetadata metadata,
	shared_ptr<const Signer> signer
	)
{
	list<shared_ptr<CPL> > cpl = cpls ();
	for (list<shared_ptr<CPL> >::const_iterator i = cpl.begin(); i != cpl.end(); ++i) {
		string const filename = (*i)->id() + "_cpl.xml";
		(*i)->write_xml (_directory / filename, standard, signer);
	}

	string const pkl_uuid = make_uuid ();
	boost::filesystem::path const pkl_path = write_pkl (standard, pkl_uuid, metadata, signer);
	
	write_volindex (standard);
	write_assetmap (standard, pkl_uuid, boost::filesystem::file_size (pkl_path), metadata);
}

list<shared_ptr<CPL> >
DCP::cpls () const
{
	return list_of_type<Asset, CPL> (_assets);
}
