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
#include <libxml++/libxml++.h>
#include "dcp.h"
#include "asset.h"
#include "sound_asset.h"
#include "picture_asset.h"
#include "util.h"
#include "metadata.h"
#include "exceptions.h"
#include "cpl.h"
#include "pkl.h"
#include "asset_map.h"

using namespace std;
using namespace boost;
using namespace libdcp;

DCP::DCP (string directory, string name, ContentKind content_kind, int fps, int length)
	: _directory (directory)
	, _name (name)
	, _content_kind (content_kind)
	, _fps (fps)
	, _length (length)
{
	
}

void
DCP::add_sound_asset (vector<string> const & files)
{
	_assets.push_back (shared_ptr<SoundAsset> (new SoundAsset (files, _directory, "audio.mxf", &Progress, _fps, _length)));
}

void
DCP::add_sound_asset (sigc::slot<string, Channel> get_path, int channels)
{
	_assets.push_back (shared_ptr<SoundAsset> (new SoundAsset (get_path, _directory, "audio.mxf", &Progress, _fps, _length, channels)));
}

void
DCP::add_picture_asset (vector<string> const & files, int width, int height)
{
	_assets.push_back (shared_ptr<PictureAsset> (new PictureAsset (files, _directory, "video.mxf", &Progress, _fps, _length, width, height)));
}

void
DCP::add_picture_asset (sigc::slot<string, int> get_path, int width, int height)
{
	_assets.push_back (shared_ptr<PictureAsset> (new PictureAsset (get_path, _directory, "video.mxf", &Progress, _fps, _length, width, height)));
}

void
DCP::write_xml () const
{
	string cpl_uuid = make_uuid ();
	string cpl_path = write_cpl (cpl_uuid);
	int cpl_length = filesystem::file_size (cpl_path);
	string cpl_digest = make_digest (cpl_path, 0);

	string pkl_uuid = make_uuid ();
	string pkl_path = write_pkl (pkl_uuid, cpl_uuid, cpl_digest, cpl_length);
	
	write_volindex ();
	write_assetmap (cpl_uuid, cpl_length, pkl_uuid, filesystem::file_size (pkl_path));
}

string
DCP::write_cpl (string cpl_uuid) const
{
	filesystem::path p;
	p /= _directory;
	stringstream s;
	s << cpl_uuid << "_cpl.xml";
	p /= s.str();
	ofstream cpl (p.string().c_str());
	
	cpl << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    << "<CompositionPlaylist xmlns=\"http://www.smpte-ra.org/schemas/429-7/2006/CPL\">\n"
	    << "  <Id>urn:uuid:" << cpl_uuid << "</Id>\n"
	    << "  <AnnotationText>" << _name << "</AnnotationText>\n"
	    << "  <IssueDate>" << Metadata::instance()->issue_date << "</IssueDate>\n"
	    << "  <Creator>" << Metadata::instance()->creator << "</Creator>\n"
	    << "  <ContentTitleText>" << _name << "</ContentTitleText>\n"
	    << "  <ContentKind>" << content_kind_to_string (_content_kind) << "</ContentKind>\n"
	    << "  <ContentVersion>\n"
	    << "    <Id>urn:uri:" << cpl_uuid << "_" << Metadata::instance()->issue_date << "</Id>\n"
	    << "    <LabelText>" << cpl_uuid << "_" << Metadata::instance()->issue_date << "</LabelText>\n"
	    << "  </ContentVersion>\n"
	    << "  <RatingList/>\n"
	    << "  <ReelList>\n";

	cpl << "    <Reel>\n"
	    << "      <Id>urn:uuid:" << make_uuid() << "</Id>\n"
	    << "      <AssetList>\n";

	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		(*i)->write_to_cpl (cpl);
	}

	cpl << "      </AssetList>\n"
	    << "    </Reel>\n"
	    << "  </ReelList>\n"
	    << "</CompositionPlaylist>\n";

	return p.string ();
}

std::string
DCP::write_pkl (string pkl_uuid, string cpl_uuid, string cpl_digest, int cpl_length) const
{
	filesystem::path p;
	p /= _directory;
	stringstream s;
	s << pkl_uuid << "_pkl.xml";
	p /= s.str();
	ofstream pkl (p.string().c_str());

	pkl << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    << "<PackingList xmlns=\"http://www.smpte-ra.org/schemas/429-8/2007/PKL\">\n"
	    << "  <Id>urn:uuid:" << pkl_uuid << "</Id>\n"
	    << "  <AnnotationText>" << _name << "</AnnotationText>\n"
	    << "  <IssueDate>" << Metadata::instance()->issue_date << "</IssueDate>\n"
	    << "  <Issuer>" << Metadata::instance()->issuer << "</Issuer>\n"
	    << "  <Creator>" << Metadata::instance()->creator << "</Creator>\n"
	    << "  <AssetList>\n";

	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		(*i)->write_to_pkl (pkl);
	}

	pkl << "    <Asset>\n"
	    << "      <Id>urn:uuid:" << cpl_uuid << "</Id>\n"
	    << "      <Hash>" << cpl_digest << "</Hash>\n"
	    << "      <Size>" << cpl_length << "</Size>\n"
	    << "      <Type>text/xml</Type>\n"
	    << "    </Asset>\n";

	pkl << "  </AssetList>\n"
	    << "</PackingList>\n";

	return p.string ();
}

void
DCP::write_volindex () const
{
	filesystem::path p;
	p /= _directory;
	p /= "VOLINDEX.xml";
	ofstream vi (p.string().c_str());

	vi << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	   << "<VolumeIndex xmlns=\"http://www.smpte-ra.org/schemas/429-9/2007/AM\">\n"
	   << "  <Index>1</Index>\n"
	   << "</VolumeIndex>\n";
}

void
DCP::write_assetmap (string cpl_uuid, int cpl_length, string pkl_uuid, int pkl_length) const
{
	filesystem::path p;
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

	am << "    <Asset>\n"
	   << "      <Id>urn:uuid:" << cpl_uuid << "</Id>\n"
	   << "      <ChunkList>\n"
	   << "        <Chunk>\n"
	   << "          <Path>" << cpl_uuid << "_cpl.xml</Path>\n"
	   << "          <VolumeIndex>1</VolumeIndex>\n"
	   << "          <Offset>0</Offset>\n"
	   << "          <Length>" << cpl_length << "</Length>\n"
	   << "        </Chunk>\n"
	   << "      </ChunkList>\n"
	   << "    </Asset>\n";
	
	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		(*i)->write_to_assetmap (am);
	}

	am << "  </AssetList>\n"
	   << "</AssetMap>\n";
}


DCP::DCP (string directory)
	: _directory (directory)
{
	string cpl_file;
	string pkl_file;
	string asset_map_file;

	for (filesystem::directory_iterator i = filesystem::directory_iterator(directory); i != filesystem::directory_iterator(); ++i) {
		
		string const t = i->path().string ();

		if (ends_with (t, ".mxf")) {
			continue;
		}

		xmlpp::DomParser* p = new xmlpp::DomParser;

		try {
			p->parse_file (t);
		} catch (std::exception& e) {
			delete p;
			continue;
		}
		
		if (!p) {
			delete p;
			continue;
		}

		string const root = p->get_document()->get_root_node()->get_name ();
		delete p;
		
		if (root == "CompositionPlaylist") {
			if (cpl_file.empty ()) {
				cpl_file = t;
			} else {
				throw DCPReadError ("duplicate CPLs found");
			}
		} else if (root == "PackingList") {
			if (pkl_file.empty ()) {
				pkl_file = t;
			} else {
				throw DCPReadError ("duplicate PKLs found");
			}
		} else if (root == "AssetMap") {
			if (asset_map_file.empty ()) {
				asset_map_file = t;
			} else {
				throw DCPReadError ("duplicate AssetMaps found");
			}
			asset_map_file = t;
		}
	}

	if (cpl_file.empty ()) {
		throw FileError ("no CPL file found", "");
	}

	if (pkl_file.empty ()) {
		throw FileError ("no PKL file found", "");
	}

	if (asset_map_file.empty ()) {
		throw FileError ("no AssetMap file found", "");
	}

	/* Read the XML */
	shared_ptr<CPL> cpl;
	try {
		cpl.reset (new CPL (cpl_file));
	} catch (FileError& e) {
		throw FileError ("could not load CPL file", cpl_file);
	}

	shared_ptr<PKL> pkl;
	try {
		pkl.reset (new PKL (pkl_file));
	} catch (FileError& e) {
		throw FileError ("could not load PKL file", pkl_file);
	}

	shared_ptr<AssetMap> asset_map;
	try {
		asset_map.reset (new AssetMap (asset_map_file));
	} catch (FileError& e) {
		throw FileError ("could not load AssetMap file", asset_map_file);
	}

	/* Cross-check */
	/* XXX */

	/* Now cherry-pick the required bits into our own data structure */
	
	_name = cpl->annotation_text;
	_content_kind = cpl->content_kind;

	shared_ptr<CPLAssetList> cpl_assets = cpl->reels.front()->asset_list;

	/* XXX */
	_fps = cpl_assets->main_picture->frame_rate.numerator;
	_length = cpl_assets->main_picture->duration;

	string n = cpl_assets->main_picture->annotation_text;
	if (n.empty ()) {
		n = pkl->asset_from_id(cpl_assets->main_picture->id)->original_file_name;
	}

	_assets.push_back (shared_ptr<PictureAsset> (
				   new PictureAsset (
					   _directory,
					   n,
					   _fps,
					   _length
					   )
				   ));

	if (cpl_assets->main_sound) {

		n = cpl_assets->main_sound->annotation_text;
		if (n.empty ()) {
			n = pkl->asset_from_id(cpl_assets->main_sound->id)->original_file_name;
		}
	
		_assets.push_back (shared_ptr<SoundAsset> (
					   new SoundAsset (
						   _directory,
						   n,
						   _fps,
						   _length
						   )
					   ));
	}
}

list<string>
DCP::equals (DCP const & other, EqualityOptions opt) const
{
	list<string> notes;
	
	if (opt.flags & LIBDCP_METADATA) {
		if (_name != other._name) {
			notes.push_back ("names differ");
		}
		if (_content_kind != other._content_kind) {
			notes.push_back ("content kinds differ");
		}
		if (_fps != other._fps) {
			notes.push_back ("frames per second differ");
		}
		if (_length != other._length) {
			notes.push_back ("lengths differ");
		}
	}

	if (_assets.size() != other._assets.size()) {
		notes.push_back ("asset counts differ");
	}
	
	list<shared_ptr<Asset> >::const_iterator a = _assets.begin ();
	list<shared_ptr<Asset> >::const_iterator b = other._assets.begin ();
	
	while (a != _assets.end ()) {
		list<string> n = (*a)->equals (*b, opt);
		notes.merge (n);
		++a;
		++b;
	}

	return notes;
}

shared_ptr<const PictureAsset>
DCP::picture_asset () const
{
	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		shared_ptr<PictureAsset> p = dynamic_pointer_cast<PictureAsset> (*i);
		if (p) {
			return p;
		}
	}

	return shared_ptr<const PictureAsset> ();
}

shared_ptr<const SoundAsset>
DCP::sound_asset () const
{
	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		shared_ptr<SoundAsset> s = dynamic_pointer_cast<SoundAsset> (*i);
		if (s) {
			return s;
		}
	}

	return shared_ptr<const SoundAsset> ();
}
