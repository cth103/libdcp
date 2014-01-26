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
#include "kdm.h"
#include "compose.hpp"
#include "AS_DCP.h"
#include <xmlsec/xmldsig.h>
#include <xmlsec/app.h>
#include <libxml++/libxml++.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <iostream>

using std::string;
using std::list;
using std::stringstream;
using std::ostream;
using std::copy;
using std::back_inserter;
using std::make_pair;
using std::map;
using boost::shared_ptr;
using boost::lexical_cast;
using boost::dynamic_pointer_cast;
using namespace dcp;

DCP::DCP (boost::filesystem::path directory)
	: _directory (directory)
{
	boost::filesystem::create_directories (directory);
}

void
DCP::read ()
{
	/* Read the ASSETMAP */
	
	boost::filesystem::path asset_map_file;
	if (boost::filesystem::exists (_directory / "ASSETMAP")) {
		asset_map_file = _directory / "ASSETMAP";
	} else if (boost::filesystem::exists (_directory / "ASSETMAP.xml")) {
		asset_map_file = _directory / "ASSETMAP.xml";
	} else {
		boost::throw_exception (DCPReadError ("could not find AssetMap file"));
	}

	cxml::Document asset_map ("AssetMap");
	asset_map.read_file (asset_map_file);
	list<shared_ptr<cxml::Node> > assets = asset_map.node_child("AssetList")->node_children ("Asset");
	map<string, boost::filesystem::path> paths;
	for (list<shared_ptr<cxml::Node> >::const_iterator i = assets.begin(); i != assets.end(); ++i) {
		if ((*i)->node_child("ChunkList")->node_children("Chunk").size() != 1) {
			boost::throw_exception (XMLError ("unsupported asset chunk count"));
		}
		paths.insert (make_pair (
			(*i)->string_child ("Id"),
			(*i)->node_child("ChunkList")->node_child("Chunk")->string_child ("Path")
				      ));
	}

	/* Read all the assets from the asset map */
	
	for (map<string, boost::filesystem::path>::const_iterator i = paths.begin(); i != paths.end(); ++i) {
		if (boost::algorithm::ends_with (i->second.string(), ".xml")) {
			xmlpp::DomParser* p = new xmlpp::DomParser;
			try {
				p->parse_file (i->second.string());
			} catch (std::exception& e) {
				delete p;
				continue;
			}
			
			string const root = p->get_document()->get_root_node()->get_name ();
			delete p;
			
			if (root == "CompositionPlaylist") {
				_assets.push_back (shared_ptr<CPL> (new CPL (i->second)));
			} else if (root == "DCSubtitle") {
				_assets.push_back (shared_ptr<SubtitleContent> (new SubtitleContent (i->second)));
			}
		} else if (boost::algorithm::ends_with (i->second.string(), ".mxf")) {
			ASDCP::EssenceType_t type;
			if (ASDCP::EssenceType (i->second.string().c_str(), type) != ASDCP::RESULT_OK) {
				throw DCPReadError ("Could not find essence type");
			}
			switch (type) {
				case ASDCP::ESS_UNKNOWN:
				case ASDCP::ESS_MPEG2_VES:
					throw DCPReadError ("MPEG2 video essences are not supported");
				case ASDCP::ESS_JPEG_2000:
					_assets.push_back (shared_ptr<MonoPictureMXF> (new MonoPictureMXF (i->second)));
					break;
				case ASDCP::ESS_PCM_24b_48k:
				case ASDCP::ESS_PCM_24b_96k:
					_assets.push_back (shared_ptr<SoundMXF> (new SoundMXF (i->second)));
					break;
				case ASDCP::ESS_JPEG_2000_S:
					_assets.push_back (shared_ptr<StereoPictureMXF> (new StereoPictureMXF (i->second)));
					break;
				default:
					throw DCPReadError ("Unknown MXF essence type");
				}
		}
	}
}

bool
DCP::equals (DCP const & other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (_assets.size() != other._assets.size()) {
		note (ERROR, "Asset counts differ");
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
DCP::add (shared_ptr<Asset> asset)
{
	_assets.push_back (asset);
}

class AssetComparator
{
public:
	bool operator() (shared_ptr<const Content> a, shared_ptr<const Content> b) {
		return a->id() < b->id();
	}
};

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
DCP::add (KDM const & kdm)
{
	list<KDMKey> keys = kdm.keys ();
	list<shared_ptr<CPL> > cpl = cpls ();
	
	for (list<shared_ptr<CPL> >::iterator i = cpl.begin(); i != cpl.end(); ++i) {
		for (list<KDMKey>::iterator j = keys.begin(); j != keys.end(); ++j) {
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
	list<shared_ptr<Asset> >::const_iterator i = _assets.begin();
	shared_ptr<CPL> first_cpl;
	while (i != _assets.end()) {
		first_cpl = dynamic_pointer_cast<CPL> (*i);
		if (first_cpl) {
			break;
		}
	}

	assert (first_cpl);
	pkl->add_child("AnnotationText")->add_child_text (first_cpl->annotation_text ());
	
	pkl->add_child("IssueDate")->add_child_text (metadata.issue_date);
	pkl->add_child("Issuer")->add_child_text (metadata.issuer);
	pkl->add_child("Creator")->add_child_text (metadata.creator);

	xmlpp::Element* asset_list = pkl->add_child("AssetList");

	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		shared_ptr<Content> c = dynamic_pointer_cast<Content> (*i);
		if (c) {
			c->write_to_pkl (asset_list);
		}
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
	if (standard == INTEROP) {
		p /= "VOLINDEX";
	} else {
		p /= "VOLINDEX.xml";
	}

	xmlpp::Document doc;
	xmlpp::Element* root = doc.create_root_node ("VolumeIndex", "http://www.smpte-ra.org/schemas/429-9/2007/AM");
	root->add_child("Index")->add_child_text ("1");
	doc.write_to_file (p.string (), "UTF-8");
}

void
DCP::write_assetmap (Standard standard, string pkl_uuid, int pkl_length, XMLMetadata metadata) const
{
	boost::filesystem::path p = _directory;
	if (standard == INTEROP) {
		p /= "ASSETMAP";
	} else {
		p /= "ASSETMAP.xml";
	}

	xmlpp::Document doc;
	xmlpp::Element* root;
	if (standard == INTEROP) {
		root = doc.create_root_node ("AssetMap", "http://www.digicine.com/PROTO-ASDCP-AM-20040311#");
	} else {
		root = doc.create_root_node ("AssetMap", "http://www.smpte-ra.org/schemas/429-9/2007/AM");
	}

	root->add_child("Id")->add_child_text ("urn:uuid:" + make_uuid());
	root->add_child("AnnotationText")->add_child_text ("Created by " + metadata.creator);
	if (standard == INTEROP) {
		root->add_child("VolumeCount")->add_child_text ("1");
		root->add_child("IssueDate")->add_child_text (metadata.issue_date);
		root->add_child("Issuer")->add_child_text (metadata.issuer);
		root->add_child("Creator")->add_child_text (metadata.creator);
	} else {
		root->add_child("Creator")->add_child_text (metadata.creator);
		root->add_child("VolumeCount")->add_child_text ("1");
		root->add_child("IssueDate")->add_child_text (metadata.issue_date);
		root->add_child("Issuer")->add_child_text (metadata.issuer);
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
	chunk->add_child("Length")->add_child_text (lexical_cast<string> (pkl_length));
	
	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		(*i)->write_to_assetmap (asset_list);
	}

	/* This must not be the _formatted version otherwise signature digests will be wrong */
	doc.write_to_file (p.string (), "UTF-8");
}

void
DCP::write_xml (
	Standard standard,
	XMLMetadata metadata,
	shared_ptr<const Signer> signer
	)
{
	string const pkl_uuid = make_uuid ();
	boost::filesystem::path const pkl_path = write_pkl (standard, pkl_uuid, metadata, signer);
	
	write_volindex (standard);
	write_assetmap (standard, pkl_uuid, boost::filesystem::file_size (pkl_path), metadata);

	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		shared_ptr<CPL> c = dynamic_pointer_cast<CPL> (*i);
		if (c) {
			string const filename = c->id() + "_cpl.xml";
			c->write_xml (_directory / filename, standard, metadata, signer);
		}
	}
}

list<shared_ptr<CPL> >
DCP::cpls () const
{
	list<shared_ptr<CPL> > cpls;
	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		shared_ptr<CPL> c = dynamic_pointer_cast<CPL> (*i);
		if (c) {
			cpls.push_back (c);
		}
	}

	return cpls;
}

	
