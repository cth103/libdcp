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
#include <boost/lexical_cast.hpp>
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

	xmlpp::Document doc;
	xmlpp::Element* pkl = doc.create_root_node("PackingList", "http://www.smpte-ra.org/schemas/429-8/2007/PKL");
	if (_encrypted) {
		pkl->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "dsig");
	}

	pkl->add_child("Id")->add_child_text ("urn:uuid:" + pkl_uuid);
	/* XXX: this is a bit of a hack */
	pkl->add_child("AnnotationText")->add_child_text(_cpls.front()->name());
	pkl->add_child("IssueDate")->add_child_text (Metadata::instance()->issue_date);
	pkl->add_child("Issuer")->add_child_text (Metadata::instance()->issuer);
	pkl->add_child("Creator")->add_child_text (Metadata::instance()->creator);

	{
		xmlpp::Element* asset_list = pkl->add_child("AssetList");
		list<shared_ptr<const Asset> > a = assets ();
		for (list<shared_ptr<const Asset> >::const_iterator i = a.begin(); i != a.end(); ++i) {
			(*i)->write_to_pkl (asset_list);
		}

		for (list<shared_ptr<const CPL> >::const_iterator i = _cpls.begin(); i != _cpls.end(); ++i) {
			(*i)->write_to_pkl (asset_list);
		}
	}

	if (_encrypted) {
		sign (pkl, _certificates, _signer_key);
	}
		
	doc.write_to_file_formatted (p.string(), "UTF-8");

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
		sign (cpl, certificates, signer_key);
	}

	doc.write_to_file_formatted (p.string(), "UTF-8");

	_digest = make_digest (p.string (), 0);
	_length = boost::filesystem::file_size (p.string ());
}

void
CPL::write_to_pkl (xmlpp::Element* p) const
{
	xmlpp::Element* asset = p->add_child("Asset");
	asset->add_child("Id")->add_child_text("urn:uuid:" + _uuid);
	asset->add_child("Hash")->add_child_text(_digest);
	asset->add_child("Size")->add_child_text(boost::lexical_cast<string> (_length));
	asset->add_child("Type")->add_child_text("text/xml");
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

shared_ptr<xmlpp::Document>
CPL::make_kdm (CertificateChain const & certificates, string const & signer_key, shared_ptr<const Certificate> recipient_cert) const
{
	shared_ptr<xmlpp::Document> doc (new xmlpp::Document);
	xmlpp::Element* root = doc->create_root_node ("DCinemaSecurityMessage");
	root->set_namespace_declaration ("http://www.smpte-ra.org/schemas/430-3/2006/ETM", "");
	root->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "ds");
	root->set_namespace_declaration ("http://www.w3.org/2001/04/xmlenc#", "enc");

	{
		xmlpp::Element* authenticated_public = root->add_child("AuthenticatedPublic");
		authenticated_public->set_attribute("Id", "ID_AuthenticatedPublic");
		xmlAddID (0, doc->cobj(), (const xmlChar *) "ID_AuthenticatedPublic", authenticated_public->get_attribute("Id")->cobj());
		
		authenticated_public->add_child("MessageId")->add_child_text("urn:uuid:" + make_uuid());
		authenticated_public->add_child("MessageType")->add_child_text("http://www.smpte-ra.org/430-1/2006/KDM#kdm-key-type");
		authenticated_public->add_child("AnnotationText")->add_child_text(Metadata::instance()->product_name);
		authenticated_public->add_child("IssueDate")->add_child_text(Metadata::instance()->issue_date);

		{
			xmlpp::Element* signer = authenticated_public->add_child("Signer");
			signer->add_child("X509IssuerName", "ds")->add_child_text (
				Certificate::name_for_xml (recipient_cert->issuer())
				);
			signer->add_child("X509SerialNumber", "ds")->add_child_text (
				recipient_cert->serial()
				);
		}

		{
			xmlpp::Element* required_extensions = authenticated_public->add_child("RequiredExtensions");

			{
				xmlpp::Element* kdm_required_extensions = required_extensions->add_child("KDMRequiredExtensions");
				kdm_required_extensions->set_namespace_declaration ("http://www.smpte-ra.org/schemas/430-1/2006/KDM");
				{
					xmlpp::Element* recipient = kdm_required_extensions->add_child("Recipient");
					{
						xmlpp::Element* serial_element = recipient->add_child("X509IssuerSerial");
						serial_element->add_child("X509IssuerName", "ds")->add_child_text (
							Certificate::name_for_xml (recipient_cert->issuer())
							);
						serial_element->add_child("X509SerialNumber", "ds")->add_child_text (
							recipient_cert->serial()
							);
					}

					recipient->add_child("X509SubjectName")->add_child_text (Certificate::name_for_xml (recipient_cert->subject()));
				}

				kdm_required_extensions->add_child("CompositionPlaylistId")->add_child_text("XXX");
				kdm_required_extensions->add_child("ContentTitleText")->add_child_text("XXX");
				kdm_required_extensions->add_child("ContentAuthenticator")->add_child_text("XXX");
				kdm_required_extensions->add_child("ContentKeysNotValidBefore")->add_child_text("XXX");
				kdm_required_extensions->add_child("ContentKeysNotValidAfter")->add_child_text("XXX");

				{
					xmlpp::Element* authorized_device_info = kdm_required_extensions->add_child("AuthorizedDeviceInfo");
					authorized_device_info->add_child("DeviceListIdentifier")->add_child_text("urn:uuid:" + make_uuid());
					authorized_device_info->add_child("DeviceListDescription")->add_child_text(recipient_cert->subject());
					{
						xmlpp::Element* device_list = authorized_device_info->add_child("DeviceList");
						device_list->add_child("CertificateThumbprint")->add_child_text("XXX");
					}
				}

				{
					xmlpp::Element* key_id_list = kdm_required_extensions->add_child("KeyIdList");
					list<shared_ptr<const Asset> > a = assets();
					for (list<shared_ptr<const Asset> >::iterator i = a.begin(); i != a.end(); ++i) {
						/* XXX: non-MXF assets? */
						shared_ptr<const MXFAsset> mxf = boost::dynamic_pointer_cast<const MXFAsset> (*i);
						if (mxf) {
							mxf->add_typed_key_id (key_id_list);
						}
					}
				}

				{
					xmlpp::Element* forensic_mark_flag_list = kdm_required_extensions->add_child("ForensicMarkFlagList");
					forensic_mark_flag_list->add_child("ForensicMarkFlag")->add_child_text ( 
						"http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-picture-disable"
						);
					forensic_mark_flag_list->add_child("ForensicMarkFlag")->add_child_text ( 
						"http://www.smpte-ra.org/430-1/2006/KDM#mrkflg-audio-disable"
						);
				}
			}
		}
					 
		authenticated_public->add_child("NonCriticalExtensions");
	}

	{
		xmlpp::Element* authenticated_private = root->add_child("AuthenticatedPrivate");
		authenticated_private->set_attribute ("Id", "ID_AuthenticatedPrivate");
		xmlAddID (0, doc->cobj(), (const xmlChar *) "ID_AuthenticatedPrivate", authenticated_private->get_attribute("Id")->cobj());
		{
			xmlpp::Element* encrypted_key = authenticated_private->add_child ("EncryptedKey", "enc");
			{
				xmlpp::Element* encryption_method = encrypted_key->add_child ("EncryptionMethod", "enc");
				encryption_method->set_attribute ("Algorithm", "http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p");
				encryption_method->add_child("DigestMethod", "ds")->set_attribute("Algorithm", "http://www.w3.org/2000/09/xmldsig#sha1");
			}

			xmlpp::Element* cipher_data = authenticated_private->add_child ("CipherData", "enc");
			cipher_data->add_child("CipherValue", "enc")->add_child_text("XXX");
		}
	}
	
	/* XXX: x2 one for each mxf? */

	{
		xmlpp::Element* signature = root->add_child("Signature", "ds");
		
		{
			xmlpp::Element* signed_info = signature->add_child("SignedInfo", "ds");
			signed_info->add_child("CanonicalizationMethod", "ds")->set_attribute(
				"Algorithm", "http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments"
				);
			signed_info->add_child("SignatureMethod", "ds")->set_attribute(
				"Algorithm", "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256"
				);
			{
				xmlpp::Element* reference = signed_info->add_child("Reference", "ds");
				reference->set_attribute("URI", "#ID_AuthenticatedPublic");
				reference->add_child("DigestMethod", "ds")->set_attribute("Algorithm", "http://www.w3.org/2001/04/xmlenc#sha256");
				reference->add_child("DigestValue", "ds");
			}
			
			{				
				xmlpp::Element* reference = signed_info->add_child("Reference", "ds");
				reference->set_attribute("URI", "#ID_AuthenticatedPrivate");
				reference->add_child("DigestMethod", "ds")->set_attribute("Algorithm", "http://www.w3.org/2001/04/xmlenc#sha256");
				reference->add_child("DigestValue", "ds");
			}
		}
		
		add_signature_value (signature, certificates, signer_key, "ds");
	}

	return doc;
}
