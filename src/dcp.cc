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
#include <fstream>
#include <iomanip>
#include <cassert>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <libxml++/libxml++.h>
#include "dcp.h"
#include "asset.h"
#include "sound_asset.h"
#include "picture_asset.h"
#include "subtitle_asset.h"
#include "util.h"
#include "metadata.h"
#include "exceptions.h"
#include "cpl_file.h"
#include "pkl_file.h"
#include "asset_map.h"
#include "reel.h"
#include "cpl.h"

using std::string;
using std::list;
using std::stringstream;
using std::ofstream;
using std::ostream;
using boost::shared_ptr;
using namespace libdcp;

DCP::DCP (string directory)
	: _directory (directory)
{
	boost::filesystem::create_directories (directory);
}

void
DCP::write_xml () const
{
	for (list<shared_ptr<const CPL> >::const_iterator i = _cpls.begin(); i != _cpls.end(); ++i) {
		(*i)->write_xml ();
	}

	string pkl_uuid = make_uuid ();
	string pkl_path = write_pkl (pkl_uuid);
	
	write_volindex ();
	write_assetmap (pkl_uuid, boost::filesystem::file_size (pkl_path));
}

std::string
DCP::write_pkl (string pkl_uuid) const
{
	assert (!_cpls.empty ());
	
	boost::filesystem::path p;
	p /= _directory;
	stringstream s;
	s << pkl_uuid << "_pkl.xml";
	p /= s.str();
	ofstream pkl (p.string().c_str());

	pkl << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    << "<PackingList xmlns=\"http://www.smpte-ra.org/schemas/429-8/2007/PKL\">\n"
	    << "  <Id>urn:uuid:" << pkl_uuid << "</Id>\n"
		/* XXX: this is a bit of a hack */
	    << "  <AnnotationText>" << _cpls.front()->name() << "</AnnotationText>\n"
	    << "  <IssueDate>" << Metadata::instance()->issue_date << "</IssueDate>\n"
	    << "  <Issuer>" << Metadata::instance()->issuer << "</Issuer>\n"
	    << "  <Creator>" << Metadata::instance()->creator << "</Creator>\n"
	    << "  <AssetList>\n";

	list<shared_ptr<const Asset> > a = assets ();
	for (list<shared_ptr<const Asset> >::const_iterator i = a.begin(); i != a.end(); ++i) {
		(*i)->write_to_pkl (pkl);
	}

	for (list<shared_ptr<const CPL> >::const_iterator i = _cpls.begin(); i != _cpls.end(); ++i) {
		(*i)->write_to_pkl (pkl);
	}

	pkl << "  </AssetList>\n"
	    << "</PackingList>\n";

	return p.string ();
}

void
DCP::write_volindex () const
{
	boost::filesystem::path p;
	p /= _directory;
	p /= "VOLINDEX.xml";
	ofstream vi (p.string().c_str());

	vi << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	   << "<VolumeIndex xmlns=\"http://www.smpte-ra.org/schemas/429-9/2007/AM\">\n"
	   << "  <Index>1</Index>\n"
	   << "</VolumeIndex>\n";
}

void
DCP::write_assetmap (string pkl_uuid, int pkl_length) const
{
	boost::filesystem::path p;
	p /= _directory;
	p /= "ASSETMAP.xml";
	ofstream am (p.string().c_str());

	am << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	   << "<AssetMap xmlns=\"http://www.smpte-ra.org/schemas/429-9/2007/AM\">\n"
	   << "  <Id>urn:uuid:" << make_uuid() << "</Id>\n"
	   << "  <Creator>" << Metadata::instance()->creator << "</Creator>\n"
	   << "  <VolumeCount>1</VolumeCount>\n"
	   << "  <IssueDate>" << Metadata::instance()->issue_date << "</IssueDate>\n"
	   << "  <Issuer>" << Metadata::instance()->issuer << "</Issuer>\n"
	   << "  <AssetList>\n";

	am << "    <Asset>\n"
	   << "      <Id>urn:uuid:" << pkl_uuid << "</Id>\n"
	   << "      <PackingList>true</PackingList>\n"
	   << "      <ChunkList>\n"
	   << "        <Chunk>\n"
	   << "          <Path>" << pkl_uuid << "_pkl.xml</Path>\n"
	   << "          <VolumeIndex>1</VolumeIndex>\n"
	   << "          <Offset>0</Offset>\n"
	   << "          <Length>" << pkl_length << "</Length>\n"
	   << "        </Chunk>\n"
	   << "      </ChunkList>\n"
	   << "    </Asset>\n";
	
	for (list<shared_ptr<const CPL> >::const_iterator i = _cpls.begin(); i != _cpls.end(); ++i) {
		(*i)->write_to_assetmap (am);
	}

	list<shared_ptr<const Asset> > a = assets ();
	for (list<shared_ptr<const Asset> >::const_iterator i = a.begin(); i != a.end(); ++i) {
		(*i)->write_to_assetmap (am);
	}

	am << "  </AssetList>\n"
	   << "</AssetMap>\n";
}


void
DCP::read (bool require_mxfs)
{
	Files files;

	shared_ptr<AssetMap> asset_map;
	try {
		boost::filesystem::path p = _directory;
		p /= "ASSETMAP";
		if (boost::filesystem::exists (p)) {
			asset_map.reset (new AssetMap (p.string ()));
		} else {
			p = _directory;
			p /= "ASSETMAP.xml";
			if (boost::filesystem::exists (p)) {
				asset_map.reset (new AssetMap (p.string ()));
			} else {
				boost::throw_exception (DCPReadError ("could not find AssetMap file"));
			}
		}
		
	} catch (FileError& e) {
		boost::throw_exception (FileError ("could not load AssetMap file", files.asset_map));
	}

	for (list<shared_ptr<AssetMapAsset> >::const_iterator i = asset_map->assets.begin(); i != asset_map->assets.end(); ++i) {
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
			files.cpls.push_back (t.string());
		} else if (root == "PackingList") {
			if (files.pkl.empty ()) {
				files.pkl = t.string();
			} else {
				boost::throw_exception (DCPReadError ("duplicate PKLs found"));
			}
		}
	}
	
	if (files.cpls.empty ()) {
		boost::throw_exception (FileError ("no CPL files found", ""));
	}

	if (files.pkl.empty ()) {
		boost::throw_exception (FileError ("no PKL file found", ""));
	}

	shared_ptr<PKLFile> pkl;
	try {
		pkl.reset (new PKLFile (files.pkl));
	} catch (FileError& e) {
		boost::throw_exception (FileError ("could not load PKL file", files.pkl));
	}

	/* Cross-check */
	/* XXX */

	for (list<string>::iterator i = files.cpls.begin(); i != files.cpls.end(); ++i) {
		_cpls.push_back (shared_ptr<CPL> (new CPL (_directory, *i, asset_map, require_mxfs)));
	}
}

bool
DCP::equals (DCP const & other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (_cpls.size() != other._cpls.size()) {
		note (ERROR, "CPL counts differ");
		return false;
	}

	list<shared_ptr<const CPL> >::const_iterator a = _cpls.begin ();
	list<shared_ptr<const CPL> >::const_iterator b = other._cpls.begin ();

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
	for (list<shared_ptr<const CPL> >::const_iterator i = _cpls.begin(); i != _cpls.end(); ++i) {
		list<shared_ptr<const Asset> > t = (*i)->assets ();
		a.merge (t);
	}

	a.sort ();
	a.unique ();
	return a;
}

