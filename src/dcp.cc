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
#include <boost/filesystem.hpp>
#include "dcp.h"
#include "asset.h"
#include "sound_asset.h"
#include "picture_asset.h"
#include "util.h"
#include "metadata.h"

using namespace std;
using namespace boost;
using namespace libdcp;

DCP::DCP (string directory, string name, ContentType content_type, int fps, int length)
	: _directory (directory)
	, _name (name)
	, _content_type (content_type)
	, _fps (fps)
	, _length (length)
{
	
}

void
DCP::add_sound_asset (vector<string> const & files)
{
	filesystem::path p;
	p /= _directory;
	p /= "audio.mxf";
	_assets.push_back (shared_ptr<SoundAsset> (new SoundAsset (files, p.string(), &Progress, _fps, _length)));
}

void
DCP::add_sound_asset (sigc::slot<string, Channel> get_path, int channels)
{
	filesystem::path p;
	p /= _directory;
	p /= "audio.mxf";
	_assets.push_back (shared_ptr<SoundAsset> (new SoundAsset (get_path, p.string(), &Progress, _fps, _length, channels)));
}

void
DCP::add_picture_asset (vector<string> const & files, int width, int height)
{
	filesystem::path p;
	p /= _directory;
	p /= "video.mxf";
	_assets.push_back (shared_ptr<PictureAsset> (new PictureAsset (files, p.string(), &Progress, _fps, _length, width, height)));
}

void
DCP::add_picture_asset (sigc::slot<string, int> get_path, int width, int height)
{
	filesystem::path p;
	p /= _directory;
	p /= "video.mxf";
	_assets.push_back (shared_ptr<PictureAsset> (new PictureAsset (get_path, p.string(), &Progress, _fps, _length, width, height)));
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
	    << "  <ContentKind>" << content_type_string (_content_type) << "</ContentKind>\n"
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
	string cpl;
	string pkl;
	string asset_map;

	for (filesystem::directory_iterator i = filesystem::directory_iterator(directory); i != filesystem::directory_iterator(); ++i) {
		string const t = i->path()->string ();
		if (ends_with (t, "_cpl.xml")) {
			if (cpl.empty ()) {
				cpl = t;
			} else {
				throw DCPReadError ("duplicate CPLs found");
			}
		} else if (ends_with (t, "_pkl.xml")) {
			if (pkl.empty ()) {
				pkl = t;
			} else {
				throw DCPReadError ("duplicate PKLs found");
			}
		} else if (ends_with (t, "ASSETMAP.xml")) {
			if (asset_map.empty ()) {
				asset_map = t;
			} else {
				throw DCPReadError ("duplicate AssetMaps found");
			}
		}
	}

	load_cpl (cpl);
	load_pkl (pkl);
	load_asset_map (asset_map);
}

void
DCP::load_cpl (string file)
{
	xmlpp::DOMParser parser;
	parser.parser_file (file);
	if (!parser) {
		throw DCPReadError ("could not create parser for CPL");
	}

	xmlpp::Element* root = parser.get_document()->get_root_node ();
	dcp_read_assert (root->get_name() == "CompositionPlaylist", "unrecognised CPL format");

	xmlpp::Node::NodeList children = root->get_children ();
	for (xmlpp::Node::NodeList::iterator i = children.begin(); i != children.end(); ++i) {
		bool taken = false;
		xml_maybe (*i, taken, _cpl_id, "Id");
		xml_maybe (*i, taken, _annotation_text, "AnnotationText");
		xml_maybe (*i, taken, _issue_date, "IssueDate");
		xml_maybe (*i, taken, _creator, "Creator");
		xml_maybe (*i, taken, _content_title_text, "ContentTitleText");
		xml_maybe (*i, taken, _content_kind, "ContentKind");

		if ((*i)->get_name() == "ContentVersion") {
			taken = true;
			load_cpl_content_version (*i);
		}

		if ((*i)->get_name() == "RatingList") {
			taken = true;
		}

		if ((*i)->get_name() == "ReelList") {
			taken = true;
			load_cpl_reel_list (*i);
		}

		xml_assert_taken (*i, taken);
	}
}

void
DCP::load_cpl_content_version (xmlpp::Node const * node)
{
	xmlpp::Node::NodeList children = node->get_children ();
	for (xmlpp::Node::NodeList::iterator i = children.begin(); i != children.end(); ++i) {
		bool taken = false;
		xml_maybe (*i, taken, _content_version_id, "Id");
		xml_maybe (*i, taken, _content_version_label_text, "LabelText");
		xml_assert_taken (*i, taken);
	}
}

void
DCP::load_cpl_reel_list (xmlpp::Node const * node)
{
	xmlpp::Node::NodeList children = node->get_children ();
	bool had_reel = false;
	for (xmlpp::Node::NodeList::iterator i = children.begin(); i != children.end(); ++i) {
		bool taken = false;
		if ((*i)->get_name() == "Reel") {
			if (!had_reel) {
				load_cpl_reel (*i);
				had_reel = true;
			} else {
				throw DCPReadError ("multiple reels not supported");
			}
		}
		xml_assert_taken (*i, taken);
	}
}

void
DCP::load_cpl_reel (xmlpp::Node const * node)
{
	xmlpp::Node::NodeList children = node->get_children ();
	for (xmlpp::Node::NodeList::iterator i = children.begin(); i != children.end(); ++i) {
		bool taken = false;
		xml_taken (*i, taken, _reel_id, "Id");
		if ((*i)->name() == "AssetList") {
			taken = true;
			load_cpl_asset_list (*i);
		}
		xml_assert_taken (*i, taken);
	}
}

void
DCP::load_cpl_asset_list (xmlpp::Node const * node)
{
	xmlpp::Node::NodeList children = node->get_children ();
	for (xmlpp::Node::NodeList::iterator i = children.begin(); i != children.end(); ++i) {
		bool taken = false;
		if ((*i)->name() == "MainPicture") {
			taken = true;
			load_cpl_main_picture (*i);
		} else if ((*i)->name() == "MainSound") {
			taken = true;
			load_cpl_main_sound (*i);
		}
		xml_assert_taken (*i, taken);
	}
}

void
DCP::load_cpl_main_picture (xmlpp::Node const * node)
{
	xmlpp::Node::NodeList children = node->get_children ();
	for (xmlpp::Node::NodeList::iterator i = children.begin(); i != children.end(); ++i) {
		bool taken = false;
		xml_maybe (*i, taken, _video_id, "Id");
		
	}
}
