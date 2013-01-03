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
#include "cpl_file.h"
#include "pkl_file.h"
#include "asset_map.h"
#include "reel.h"

using std::string;
using std::list;
using std::stringstream;
using std::ofstream;
using std::ostream;
using boost::shared_ptr;
using namespace libdcp;

DCP::DCP (string directory)
	: _directory (directory)
	, _encrypted (false)
{
	boost::filesystem::create_directories (directory);
}

void
DCP::write_xml () const
{
	for (list<shared_ptr<const CPL> >::const_iterator i = _cpls.begin(); i != _cpls.end(); ++i) {
		(*i)->write_xml (_encrypted, _certificates, _signer_key);
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
				throw DCPReadError ("could not find AssetMap file");
			}
		}
		
	} catch (FileError& e) {
		throw FileError ("could not load AssetMap file", files.asset_map);
	}

	for (list<shared_ptr<AssetMapAsset> >::const_iterator i = asset_map->assets.begin(); i != asset_map->assets.end(); ++i) {
		if ((*i)->chunks.size() != 1) {
			throw XMLError ("unsupported asset chunk count");
		}

		boost::filesystem::path t = _directory;
		t /= (*i)->chunks.front()->path;
		
		if (ends_with (t.string(), ".mxf") || ends_with (t.string(), ".ttf")) {
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
				throw DCPReadError ("duplicate PKLs found");
			}
		}
	}
	
	if (files.cpls.empty ()) {
		throw FileError ("no CPL files found", "");
	}

	if (files.pkl.empty ()) {
		throw FileError ("no PKL file found", "");
	}

	shared_ptr<PKLFile> pkl;
	try {
		pkl.reset (new PKLFile (files.pkl));
	} catch (FileError& e) {
		throw FileError ("could not load PKL file", files.pkl);
	}

	/* Cross-check */
	/* XXX */

	for (list<string>::iterator i = files.cpls.begin(); i != files.cpls.end(); ++i) {
		_cpls.push_back (shared_ptr<CPL> (new CPL (_directory, *i, asset_map, require_mxfs)));
	}
}

bool
DCP::equals (DCP const & other, EqualityOptions opt, list<string>& notes) const
{
	if (_cpls.size() != other._cpls.size()) {
		notes.push_back ("CPL counts differ");
		return false;
	}

	list<shared_ptr<const CPL> >::const_iterator a = _cpls.begin ();
	list<shared_ptr<const CPL> >::const_iterator b = other._cpls.begin ();

	while (a != _cpls.end ()) {
		if (!(*a)->equals (*b->get(), opt, notes)) {
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

	a.sort (AssetComparator ());
	a.unique ();
	return a;
}

CPL::CPL (string directory, string name, ContentKind content_kind, int length, int frames_per_second)
	: _directory (directory)
	, _name (name)
	, _content_kind (content_kind)
	, _length (length)
	, _fps (frames_per_second)
{
	_uuid = make_uuid ();
}

CPL::CPL (string directory, string file, shared_ptr<const AssetMap> asset_map, bool require_mxfs)
	: _directory (directory)
	, _content_kind (FEATURE)
	, _length (0)
	, _fps (0)
{
	/* Read the XML */
	shared_ptr<CPLFile> cpl;
	try {
		cpl.reset (new CPLFile (file));
	} catch (FileError& e) {
		throw FileError ("could not load CPL file", file);
	}
	
	/* Now cherry-pick the required bits into our own data structure */
	
	_name = cpl->annotation_text;
	_content_kind = cpl->content_kind;

	for (list<shared_ptr<CPLReel> >::iterator i = cpl->reels.begin(); i != cpl->reels.end(); ++i) {

		shared_ptr<Picture> p;

		if ((*i)->asset_list->main_picture) {
			p = (*i)->asset_list->main_picture;
		} else {
			p = (*i)->asset_list->main_stereoscopic_picture;
		}
		
		_fps = p->edit_rate.numerator;
		_length += p->duration;

		shared_ptr<PictureAsset> picture;
		shared_ptr<SoundAsset> sound;
		shared_ptr<SubtitleAsset> subtitle;

		/* Some rather twisted logic to decide if we are 3D or not;
		   some DCPs give a MainStereoscopicPicture to indicate 3D, others
		   just have a FrameRate twice the EditRate and apparently
		   expect you to divine the fact that they are hence 3D.
		*/

		if (!(*i)->asset_list->main_stereoscopic_picture && p->edit_rate == p->frame_rate) {

			try {
				picture.reset (new MonoPictureAsset (
						       _directory,
						       asset_map->asset_from_id (p->id)->chunks.front()->path,
						       _fps,
						       (*i)->asset_list->main_picture->entry_point,
						       (*i)->asset_list->main_picture->duration
						       )
					);
			} catch (MXFFileError) {
				if (require_mxfs) {
					throw;
				}
			}
			
		} else {

			try {
				picture.reset (new StereoPictureAsset (
						       _directory,
						       asset_map->asset_from_id (p->id)->chunks.front()->path,
						       _fps,
						       p->entry_point,
						       p->duration
						       )
					);
			} catch (MXFFileError) {
				if (require_mxfs) {
					throw;
				}
			}
			
		}
		
		if ((*i)->asset_list->main_sound) {
			
			try {
				sound.reset (new SoundAsset (
						     _directory,
						     asset_map->asset_from_id ((*i)->asset_list->main_sound->id)->chunks.front()->path,
						     _fps,
						     (*i)->asset_list->main_sound->entry_point,
						     (*i)->asset_list->main_sound->duration
						     )
					);
			} catch (MXFFileError) {
				if (require_mxfs) {
					throw;
				}
			}
		}

		if ((*i)->asset_list->main_subtitle) {
			
			subtitle.reset (new SubtitleAsset (
						_directory,
						asset_map->asset_from_id ((*i)->asset_list->main_subtitle->id)->chunks.front()->path
						)
				);
		}
			
		_reels.push_back (shared_ptr<Reel> (new Reel (picture, sound, subtitle)));
	}
}

void
CPL::add_reel (shared_ptr<const Reel> reel)
{
	_reels.push_back (reel);
}

void
CPL::write_xml (bool encrypted, CertificateChain const & certificates, string const & signer_key) const
{
	boost::filesystem::path p;
	p /= _directory;
	stringstream s;
	s << _uuid << "_cpl.xml";
	p /= s.str();

	xmlpp::Document doc;
	xmlpp::Element* cpl = doc.create_root_node("CompositionPlaylist", "http://www.smpte-ra.org/schemas/429-7/2006/CPL");

	if (encrypted) {
		cpl->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "dsig");
	}

	cpl->add_child("Id")->add_child_text ("urn:uuid:" + _uuid);
	cpl->add_child("AnnotationText")->add_child_text (_name);
	cpl->add_child("IssueDate")->add_child_text (Metadata::instance()->issue_date);
	cpl->add_child("Creator")->add_child_text (Metadata::instance()->creator);
	cpl->add_child("ContentTitleText")->add_child_text (_name);
	cpl->add_child("ContentKind")->add_child_text (content_kind_to_string (_content_kind));

	{
		xmlpp::Element* cv = cpl->add_child ("ContentVersion");
		cv->add_child("Id")->add_child_text ("urn:uri:" + _uuid + "_" + Metadata::instance()->issue_date);
		cv->add_child("LabelText")->add_child_text (_uuid + "_" + Metadata::instance()->issue_date);
	}

	cpl->add_child("RatingList");

	xmlpp::Element* reel_list = cpl->add_child("ReelList");
	for (list<shared_ptr<const Reel> >::const_iterator i = _reels.begin(); i != _reels.end(); ++i) {
		(*i)->write_to_cpl (reel_list);
	}

	if (encrypted) {
		xmlpp::Element* signer = cpl->add_child("Signer");
		{
			xmlpp::Element* data = signer->add_child("X509Data", "dsig");
			{
				xmlpp::Element* serial = data->add_child("X509IssuerSerial", "dsig");
				serial->add_child("X509IssuerName", "dsig")->add_child_text (
					Certificate::name_for_xml (certificates.leaf()->issuer())
					);
				serial->add_child("X509SerialNumber", "dsig")->add_child_text (
					certificates.leaf()->serial()
					);
			}
			data->add_child("X509SubjectName", "dsig")->add_child_text (
				Certificate::name_for_xml (certificates.leaf()->subject())
				);
		}

		xmlpp::Element* signature = cpl->add_child("Signature", "dsig");
		
		{
			xmlpp::Element* signed_info = signature->add_child ("SignedInfo", "dsig");
			signed_info->add_child("CanonicalizationMethod", "dsig")->set_attribute ("Algorithm", "http://www.w3.org/TR/2001/REC-xml-c14n-20010315");
			signed_info->add_child("SignatureMethod", "dsig")->set_attribute("Algorithm", "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256");
			{
				xmlpp::Element* reference = signed_info->add_child("Reference", "dsig");
				reference->set_attribute ("URI", "");
				{
					xmlpp::Element* transforms = reference->add_child("Transforms", "dsig");
					transforms->add_child("Transform", "dsig")->set_attribute (
						"Algorithm", "http://www.w3.org/2000/09/xmldsig#enveloped-signature"
						);
				}
				reference->add_child("DigestMethod", "dsig")->set_attribute("Algorithm", "http://www.w3.org/2000/09/xmldsig#sha1");
				/* This will be filled in by the signing later */
				reference->add_child("DigestValue", "dsig");
			}
		}

		signature->add_child("SignatureValue", "dsig");

		xmlpp::Element* key_info = signature->add_child("KeyInfo", "dsig");
		list<shared_ptr<Certificate> > c = certificates.leaf_to_root ();
		for (list<shared_ptr<Certificate> >::iterator i = c.begin(); i != c.end(); ++i) {
			xmlpp::Element* data = key_info->add_child("X509Data", "dsig");
			{
				xmlpp::Element* serial = data->add_child("X509IssuerSerial", "dsig");
				serial->add_child("X509IssuerName", "dsig")->add_child_text(
					Certificate::name_for_xml ((*i)->issuer())
					);
				serial->add_child("X509SerialNumber", "dsig")->add_child_text((*i)->serial());
			}
		}

		xmlSecKeysMngrPtr keys_manager = xmlSecKeysMngrCreate();
		if (!keys_manager) {
			throw MiscError ("could not create keys manager");
		}
		if (xmlSecCryptoAppDefaultKeysMngrInit (keys_manager) < 0) {
			throw MiscError ("could not initialise keys manager");
		}

		xmlSecKeyPtr const key = xmlSecCryptoAppKeyLoad (signer_key.c_str(), xmlSecKeyDataFormatPem, 0, 0, 0);
		if (key == 0) {
			throw MiscError ("could not load signer key");
		}

		if (xmlSecCryptoAppDefaultKeysMngrAdoptKey (keys_manager, key) < 0) {
			xmlSecKeyDestroy (key);
			throw MiscError ("could not use signer key");
		}

		xmlSecDSigCtx signature_context;

		if (xmlSecDSigCtxInitialize (&signature_context, keys_manager) < 0) {
			throw MiscError ("could not initialise XMLSEC context");
		}

		if (xmlSecDSigCtxSign (&signature_context, signature->cobj()) < 0) {
			throw MiscError ("could not sign CPL");
		}

		xmlSecDSigCtxFinalize (&signature_context);
		xmlSecKeysMngrDestroy (keys_manager);
	}

	doc.write_to_file_formatted (p.string(), "UTF-8");

	_digest = make_digest (p.string (), 0);
	_length = boost::filesystem::file_size (p.string ());
}

void
CPL::write_to_pkl (ostream& s) const
{
	s << "    <Asset>\n"
	  << "      <Id>urn:uuid:" << _uuid << "</Id>\n"
	  << "      <Hash>" << _digest << "</Hash>\n"
	  << "      <Size>" << _length << "</Size>\n"
	  << "      <Type>text/xml</Type>\n"
	  << "    </Asset>\n";
}

list<shared_ptr<const Asset> >
CPL::assets () const
{
	list<shared_ptr<const Asset> > a;
	for (list<shared_ptr<const Reel> >::const_iterator i = _reels.begin(); i != _reels.end(); ++i) {
		if ((*i)->main_picture ()) {
			a.push_back ((*i)->main_picture ());
		}
		if ((*i)->main_sound ()) {
			a.push_back ((*i)->main_sound ());
		}
		if ((*i)->main_subtitle ()) {
			a.push_back ((*i)->main_subtitle ());
		}
	}

	return a;
}

void
CPL::write_to_assetmap (ostream& s) const
{
	s << "    <Asset>\n"
	  << "      <Id>urn:uuid:" << _uuid << "</Id>\n"
	  << "      <ChunkList>\n"
	  << "        <Chunk>\n"
	  << "          <Path>" << _uuid << "_cpl.xml</Path>\n"
	  << "          <VolumeIndex>1</VolumeIndex>\n"
	  << "          <Offset>0</Offset>\n"
	  << "          <Length>" << _length << "</Length>\n"
	  << "        </Chunk>\n"
	  << "      </ChunkList>\n"
	  << "    </Asset>\n";
}
	
	
	
bool
CPL::equals (CPL const & other, EqualityOptions opt, list<string>& notes) const
{
	if (_name != other._name) {
		notes.push_back ("names differ");
		return false;
	}

	if (_content_kind != other._content_kind) {
		notes.push_back ("content kinds differ");
		return false;
	}

	if (_fps != other._fps) {
		notes.push_back ("frames per second differ");
		return false;
	}

	if (_length != other._length) {
		notes.push_back ("lengths differ");
		return false;
	}

	if (_reels.size() != other._reels.size()) {
		notes.push_back ("reel counts differ");
		return false;
	}
	
	list<shared_ptr<const Reel> >::const_iterator a = _reels.begin ();
	list<shared_ptr<const Reel> >::const_iterator b = other._reels.begin ();
	
	while (a != _reels.end ()) {
		if (!(*a)->equals (*b, opt, notes)) {
			return false;
		}
		++a;
		++b;
	}

	return true;
}
