/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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
 *  @brief A class to create a DCP.
 */

#include <sstream>
#include <iomanip>
#include <cassert>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <libxml++/libxml++.h>
#include <xmlsec/xmldsig.h>
#include <xmlsec/app.h>
#include "dcp.h"
#include "asset.h"
#include "sound_asset.h"
#include "picture_asset.h"
#include "subtitle_asset.h"
#include "util.h"
#include "metadata.h"
#include "exceptions.h"
#include "parse/pkl.h"
#include "parse/asset_map.h"
#include "reel.h"
#include "cpl.h"
#include "signer.h"
#include "kdm.h"
#include "raw_convert.h"

using std::string;
using std::list;
using std::stringstream;
using std::ostream;
using std::copy;
using std::back_inserter;
using std::make_pair;
using boost::shared_ptr;
using namespace libdcp;

DCP::DCP (boost::filesystem::path directory)
	: _directory (directory)
{
	boost::filesystem::create_directories (directory);
}

void
DCP::write_xml (bool interop, XMLMetadata const & metadata, shared_ptr<const Signer> signer) const
{
	for (list<shared_ptr<CPL> >::const_iterator i = _cpls.begin(); i != _cpls.end(); ++i) {
		(*i)->write_xml (interop, metadata, signer);
	}

	string pkl_uuid = make_uuid ();
	string pkl_path = write_pkl (pkl_uuid, interop, metadata, signer);
	
	write_volindex (interop);
	write_assetmap (pkl_uuid, boost::filesystem::file_size (pkl_path), interop, metadata);
}

std::string
DCP::write_pkl (string pkl_uuid, bool interop, XMLMetadata const & metadata, shared_ptr<const Signer> signer) const
{
	assert (!_cpls.empty ());
	
	boost::filesystem::path p;
	p /= _directory;
	stringstream s;
	s << pkl_uuid << "_pkl.xml";
	p /= s.str();

	xmlpp::Document doc;
	xmlpp::Element* pkl;
	if (interop) {
		pkl = doc.create_root_node("PackingList", "http://www.digicine.com/PROTO-ASDCP-PKL-20040311#");
	} else {
		pkl = doc.create_root_node("PackingList", "http://www.smpte-ra.org/schemas/429-8/2007/PKL");
	}
	
	if (signer) {
		pkl->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "dsig");
	}

	pkl->add_child("Id")->add_child_text ("urn:uuid:" + pkl_uuid);
	/* XXX: this is a bit of a hack */
	pkl->add_child("AnnotationText")->add_child_text(_cpls.front()->name());
	pkl->add_child("IssueDate")->add_child_text (metadata.issue_date);
	pkl->add_child("Issuer")->add_child_text (metadata.issuer);
	pkl->add_child("Creator")->add_child_text (metadata.creator);

	xmlpp::Element* asset_list = pkl->add_child("AssetList");
	list<shared_ptr<const Asset> > a = assets ();
	for (list<shared_ptr<const Asset> >::const_iterator i = a.begin(); i != a.end(); ++i) {
		(*i)->write_to_pkl (asset_list, interop);
	}
	
	for (list<shared_ptr<CPL> >::const_iterator i = _cpls.begin(); i != _cpls.end(); ++i) {
		(*i)->write_to_pkl (asset_list, interop);
	}

	if (signer) {
		signer->sign (pkl, interop);
	}
		
	doc.write_to_file (p.string (), "UTF-8");
	return p.string ();
}

void
DCP::write_volindex (bool interop) const
{
	boost::filesystem::path p;
	p /= _directory;
	if (interop) {
		p /= "VOLINDEX";
	} else {
		p /= "VOLINDEX.xml";
	}

	xmlpp::Document doc;
	xmlpp::Element* root;
	if (interop) {
		root = doc.create_root_node ("VolumeIndex", "http://www.digicine.com/PROTO-ASDCP-AM-20040311#");
	} else {
		root = doc.create_root_node ("VolumeIndex", "http://www.smpte-ra.org/schemas/429-9/2007/AM");
	}
	root->add_child("Index")->add_child_text ("1");
	doc.write_to_file (p.string (), "UTF-8");
}

void
DCP::write_assetmap (string pkl_uuid, int pkl_length, bool interop, XMLMetadata const & metadata) const
{
	boost::filesystem::path p;
	p /= _directory;
	if (interop) {
		p /= "ASSETMAP";
	} else {
		p /= "ASSETMAP.xml";
	}

	xmlpp::Document doc;
	xmlpp::Element* root;
	if (interop) {
		root = doc.create_root_node ("AssetMap", "http://www.digicine.com/PROTO-ASDCP-AM-20040311#");
	} else {
		root = doc.create_root_node ("AssetMap", "http://www.smpte-ra.org/schemas/429-9/2007/AM");
	}

	root->add_child("Id")->add_child_text ("urn:uuid:" + make_uuid());
	root->add_child("AnnotationText")->add_child_text ("Created by " + metadata.creator);
	if (interop) {
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
	chunk->add_child("Length")->add_child_text (raw_convert<string> (pkl_length));
	
	for (list<shared_ptr<CPL> >::const_iterator i = _cpls.begin(); i != _cpls.end(); ++i) {
		(*i)->write_to_assetmap (asset_list);
	}

	list<shared_ptr<const Asset> > a = assets ();
	for (list<shared_ptr<const Asset> >::const_iterator i = a.begin(); i != a.end(); ++i) {
		(*i)->write_to_assetmap (asset_list);
	}

	/* This must not be the _formatted version otherwise signature digests will be wrong */
	doc.write_to_file (p.string (), "UTF-8");
}

void
DCP::read (bool require_mxfs)
{
	read_assets ();
	read_cpls (require_mxfs);
}

void
DCP::read_assets ()
{
	shared_ptr<parse::AssetMap> asset_map;
	try {
		boost::filesystem::path p = _directory;
		p /= "ASSETMAP";
		if (boost::filesystem::exists (p)) {
			asset_map.reset (new libdcp::parse::AssetMap (p.string ()));
		} else {
			p = _directory;
			p /= "ASSETMAP.xml";
			if (boost::filesystem::exists (p)) {
				asset_map.reset (new libdcp::parse::AssetMap (p.string ()));
			} else {
				boost::throw_exception (FileError ("could not find AssetMap file", p, -1));
			}
		}
		
	} catch (FileError& e) {
		boost::throw_exception (FileError ("could not load AssetMap file", e.filename(), e.number ()));
	}

	for (list<shared_ptr<libdcp::parse::AssetMapAsset> >::const_iterator i = asset_map->assets.begin(); i != asset_map->assets.end(); ++i) {
		if ((*i)->chunks.size() != 1) {
			boost::throw_exception (XMLError ("unsupported asset chunk count"));
		}

		boost::filesystem::path t = _directory;
		t /= (*i)->chunks.front()->path;
		
		if (boost::algorithm::ends_with (t.string(), ".mxf") || boost::algorithm::ends_with (t.string(), ".ttf")) {
			continue;
		}

		xmlpp::DomParser* p = new xmlpp::DomParser;
		try {
			p->parse_file (t.string());
		} catch (std::exception& e) {
			delete p;
			continue;
		}

		string const root = p->get_document()->get_root_node()->get_name ();
		delete p;

		if (root == "CompositionPlaylist") {
			_files.cpls.push_back (t.string());
		} else if (root == "PackingList") {
			if (_files.pkl.empty ()) {
				_files.pkl = t.string();
			} else {
				boost::throw_exception (DCPReadError ("duplicate PKLs found"));
			}
		}
	}
	
	if (_files.cpls.empty ()) {
		boost::throw_exception (DCPReadError ("no CPL files found"));
	}

	if (_files.pkl.empty ()) {
		boost::throw_exception (DCPReadError ("no PKL file found"));
	}

	shared_ptr<parse::PKL> pkl;
	try {
		pkl.reset (new parse::PKL (_files.pkl));
	} catch (FileError& e) {
		boost::throw_exception (FileError ("could not load PKL file", _files.pkl, e.number ()));
	}

	_asset_maps.push_back (make_pair (boost::filesystem::absolute (_directory).string(), asset_map));
}

void
DCP::read_cpls (bool require_mxfs)
{
	for (list<string>::iterator i = _files.cpls.begin(); i != _files.cpls.end(); ++i) {
		_cpls.push_back (shared_ptr<CPL> (new CPL (_directory, *i, _asset_maps, require_mxfs)));
	}
}

bool
DCP::equals (DCP const & other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (_cpls.size() != other._cpls.size()) {
		note (ERROR, "CPL counts differ");
		return false;
	}

	list<shared_ptr<CPL> >::const_iterator a = _cpls.begin ();
	list<shared_ptr<CPL> >::const_iterator b = other._cpls.begin ();

	while (a != _cpls.end ()) {
		if (!(*a)->equals (*b->get(), opt, note)) {
			return false;
		}
		++a;
		++b;
	}

	return true;
}

void
DCP::add_cpl (shared_ptr<CPL> cpl)
{
	_cpls.push_back (cpl);
}

class AssetComparator
{
public:
	bool operator() (shared_ptr<const Asset> a, shared_ptr<const Asset> b) {
		return a->uuid() < b->uuid();
	}
};

list<shared_ptr<const Asset> >
DCP::assets () const
{
	list<shared_ptr<const Asset> > a;
	for (list<shared_ptr<CPL> >::const_iterator i = _cpls.begin(); i != _cpls.end(); ++i) {
		list<shared_ptr<const Asset> > t = (*i)->assets ();
		a.merge (t);
	}

	a.sort (AssetComparator ());
	a.unique ();
	return a;
}

bool
DCP::encrypted () const
{
	for (list<shared_ptr<CPL> >::const_iterator i = _cpls.begin(); i != _cpls.end(); ++i) {
		if ((*i)->encrypted ()) {
			return true;
		}
	}

	return false;
}

void
DCP::add_kdm (KDM const & kdm)
{
	list<KDMKey> keys = kdm.keys ();
	
	for (list<shared_ptr<CPL> >::iterator i = _cpls.begin(); i != _cpls.end(); ++i) {
		for (list<KDMKey>::iterator j = keys.begin(); j != keys.end(); ++j) {
			if (j->cpl_id() == (*i)->id()) {
				(*i)->add_kdm (kdm);
			}				
		}
	}
}

void
DCP::add_assets_from (DCP const & ov)
{
	copy (ov._asset_maps.begin(), ov._asset_maps.end(), back_inserter (_asset_maps));
}
