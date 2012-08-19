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
#include "subtitle_asset.h"
#include "util.h"
#include "metadata.h"
#include "exceptions.h"
#include "cpl.h"
#include "pkl.h"
#include "asset_map.h"
#include "reel.h"

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
DCP::add_reel (shared_ptr<const Reel> reel)
{
	_reels.push_back (reel);
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

	for (list<shared_ptr<const Reel> >::const_iterator i = _reels.begin(); i != _reels.end(); ++i) {
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

	for (list<shared_ptr<const Reel> >::const_iterator i = _reels.begin(); i != _reels.end(); ++i) {
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
	
	for (list<shared_ptr<const Reel> >::const_iterator i = _reels.begin(); i != _reels.end(); ++i) {
		(*i)->write_to_assetmap (am);
	}

	am << "  </AssetList>\n"
	   << "</AssetMap>\n";
}


DCP::DCP (string directory)
	: _directory (directory)
{
	Files files;
	scan (files, directory);

	if (files.cpl.empty ()) {
		throw FileError ("no CPL file found", "");
	}

	if (files.pkl.empty ()) {
		throw FileError ("no PKL file found", "");
	}

	if (files.asset_map.empty ()) {
		throw FileError ("no AssetMap file found", "");
	}

	/* Read the XML */
	shared_ptr<CPL> cpl;
	try {
		cpl.reset (new CPL (files.cpl));
	} catch (FileError& e) {
		throw FileError ("could not load CPL file", files.cpl);
	}

	shared_ptr<PKL> pkl;
	try {
		pkl.reset (new PKL (files.pkl));
	} catch (FileError& e) {
		throw FileError ("could not load PKL file", files.pkl);
	}

	shared_ptr<AssetMap> asset_map;
	try {
		asset_map.reset (new AssetMap (files.asset_map));
	} catch (FileError& e) {
		throw FileError ("could not load AssetMap file", files.asset_map);
	}

	/* Cross-check */
	/* XXX */

	/* Now cherry-pick the required bits into our own data structure */
	
	_name = cpl->annotation_text;
	_content_kind = cpl->content_kind;
	_length = 0;
	_fps = 0;

	for (list<shared_ptr<CPLReel> >::iterator i = cpl->reels.begin(); i != cpl->reels.end(); ++i) {
		assert (_fps == 0 || _fps == (*i)->asset_list->main_picture->frame_rate.numerator);
		_fps = (*i)->asset_list->main_picture->frame_rate.numerator;
		_length += (*i)->asset_list->main_picture->duration;

		string n = pkl->asset_from_id ((*i)->asset_list->main_picture->id)->original_file_name;
		if (n.empty ()) {
			n = (*i)->asset_list->main_picture->annotation_text;
		}
		
		shared_ptr<PictureAsset> picture (new PictureAsset (
							  _directory,
							  n,
							  _fps,
							  (*i)->asset_list->main_picture->duration
							  )
			);

		shared_ptr<SoundAsset> sound;
		
		if ((*i)->asset_list->main_sound) {
			
			n = pkl->asset_from_id ((*i)->asset_list->main_sound->id)->original_file_name;
			if (n.empty ()) {
				n = (*i)->asset_list->main_sound->annotation_text;
			}
			
			sound.reset (new SoundAsset (
					     _directory,
					     n,
					     _fps,
					     (*i)->asset_list->main_sound->duration
					     )
				);
		}

		assert (files.subtitles.size() < 2);

		shared_ptr<SubtitleAsset> subtitle;
		if (!files.subtitles.empty ()) {
			string const l = files.subtitles.front().substr (_directory.length ());
			subtitle.reset (new SubtitleAsset (_directory, l));
		}

		_reels.push_back (shared_ptr<Reel> (new Reel (picture, sound, subtitle)));
	}
}


void
DCP::scan (Files& files, string directory) const
{
	for (filesystem::directory_iterator i = filesystem::directory_iterator(directory); i != filesystem::directory_iterator(); ++i) {
		
		string const t = i->path().string ();

		if (filesystem::is_directory (*i)) {
			scan (files, t);
			continue;
		}

		if (ends_with (t, ".mxf") || ends_with (t, ".ttf")) {
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
			if (files.cpl.empty ()) {
				files.cpl = t;
			} else {
				throw DCPReadError ("duplicate CPLs found");
			}
		} else if (root == "PackingList") {
			if (files.pkl.empty ()) {
				files.pkl = t;
			} else {
				throw DCPReadError ("duplicate PKLs found");
			}
		} else if (root == "AssetMap") {
			if (files.asset_map.empty ()) {
				files.asset_map = t;
			} else {
				throw DCPReadError ("duplicate AssetMaps found");
			}
			files.asset_map = t;
		} else if (root == "DCSubtitle") {
			files.subtitles.push_back (t);
		}
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

	if (_reels.size() != other._reels.size()) {
		notes.push_back ("reel counts differ");
	}
	
	list<shared_ptr<const Reel> >::const_iterator a = _reels.begin ();
	list<shared_ptr<const Reel> >::const_iterator b = other._reels.begin ();
	
	while (a != _reels.end ()) {
		list<string> n = (*a)->equals (*b, opt);
		notes.merge (n);
		++a;
		++b;
	}

	return notes;
}

