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

#include <fstream>
#include <libxml/parser.h>
#include "cpl.h"
#include "parse/cpl.h"
#include "util.h"
#include "picture_asset.h"
#include "sound_asset.h"
#include "subtitle_asset.h"
#include "parse/asset_map.h"
#include "reel.h"
#include "metadata.h"
#include "encryption.h"
#include "exceptions.h"
#include "compose.hpp"

using std::string;
using std::stringstream;
using std::ofstream;
using std::ostream;
using std::list;
using boost::shared_ptr;
using boost::lexical_cast;
using namespace libdcp;

CPL::CPL (string directory, string name, ContentKind content_kind, int length, int frames_per_second)
	: _directory (directory)
	, _name (name)
	, _content_kind (content_kind)
	, _length (length)
	, _fps (frames_per_second)
{
	_id = make_uuid ();
}

/** Construct a CPL object from a XML file.
 *  @param directory The directory containing this CPL's DCP.
 *  @param file The CPL XML filename.
 *  @param asset_maps AssetMaps to look for assets in.
 *  @param require_mxfs true to throw an exception if a required MXF file does not exist.
 */
CPL::CPL (string directory, string file, list<shared_ptr<const parse::AssetMap> > asset_maps, bool require_mxfs)
	: _directory (directory)
	, _content_kind (FEATURE)
	, _length (0)
	, _fps (0)
{
	/* Read the XML */
	shared_ptr<parse::CPL> cpl;
	try {
		cpl.reset (new parse::CPL (file));
	} catch (FileError& e) {
		boost::throw_exception (FileError ("could not load CPL file", file));
	}
	
	/* Now cherry-pick the required bits into our own data structure */
	
	_name = cpl->annotation_text;
	_content_kind = cpl->content_kind;

	/* Trim urn:uuid: off the front */
	_id = cpl->id.substr (9);

	for (list<shared_ptr<parse::Reel> >::iterator i = cpl->reels.begin(); i != cpl->reels.end(); ++i) {

		shared_ptr<parse::Picture> p;

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
						       asset_from_id (asset_maps, p->id)->chunks.front()->path
						       )
					);

				picture->set_entry_point (p->entry_point);
				picture->set_duration (p->duration);
				if (p->key_id.length() > 9) {
					/* Trim urn:uuid: */
					picture->set_key_id (p->key_id.substr (9));
				}
			} catch (MXFFileError) {
				if (require_mxfs) {
					throw;
				}
			}
			
		} else {
			try {
				picture.reset (new StereoPictureAsset (
						       _directory,
						       asset_from_id (asset_maps, p->id)->chunks.front()->path,
						       _fps,
						       p->duration
						       )
					);

				picture->set_entry_point (p->entry_point);
				picture->set_duration (p->duration);
				if (p->key_id.length() > 9) {
					/* Trim urn:uuid: */
					picture->set_key_id (p->key_id.substr (9));
				}
				
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
						     asset_from_id (asset_maps, (*i)->asset_list->main_sound->id)->chunks.front()->path
						     )
					);

				shared_ptr<parse::MainSound> s = (*i)->asset_list->main_sound;

				sound->set_entry_point (s->entry_point);
				sound->set_duration (s->duration);
				if (s->key_id.length() > 9) {
					/* Trim urn:uuid: */
					sound->set_key_id (s->key_id.substr (9));
				}
			} catch (MXFFileError) {
				if (require_mxfs) {
					throw;
				}
			}
		}

		if ((*i)->asset_list->main_subtitle) {
			
			subtitle.reset (new SubtitleAsset (
						_directory,
					        asset_from_id (asset_maps, (*i)->asset_list->main_subtitle->id)->chunks.front()->path
						)
				);

			subtitle->set_entry_point ((*i)->asset_list->main_subtitle->entry_point);
			subtitle->set_duration ((*i)->asset_list->main_subtitle->duration);
		}
			
		_reels.push_back (shared_ptr<Reel> (new Reel (picture, sound, subtitle)));
	}
}

void
CPL::add_reel (shared_ptr<Reel> reel)
{
	_reels.push_back (reel);
}

void
CPL::write_xml (bool interop, XMLMetadata const & metadata, shared_ptr<Encryption> crypt) const
{
	boost::filesystem::path p;
	p /= _directory;
	stringstream s;
	s << _id << "_cpl.xml";
	p /= s.str();

	xmlpp::Document doc;
	xmlpp::Element* root;
	if (interop) {
		root = doc.create_root_node ("CompositionPlaylist", "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#");
	} else {
		root = doc.create_root_node ("CompositionPlaylist", "http://www.smpte-ra.org/schemas/429-7/2006/CPL");
	}

	if (crypt) {
		root->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "dsig");
	}
	
	root->add_child("Id")->add_child_text ("urn:uuid:" + _id);
	root->add_child("AnnotationText")->add_child_text (_name);
	root->add_child("IssueDate")->add_child_text (metadata.issue_date);
	root->add_child("Issuer")->add_child_text (metadata.issuer);
	root->add_child("Creator")->add_child_text (metadata.creator);
	root->add_child("ContentTitleText")->add_child_text (_name);
	root->add_child("ContentKind")->add_child_text (content_kind_to_string (_content_kind));
	{
		xmlpp::Node* cv = root->add_child ("ContentVersion");
		cv->add_child ("Id")->add_child_text ("urn:uri:" + _id + "_" + metadata.issue_date);
		cv->add_child ("LabelText")->add_child_text (_id + "_" + metadata.issue_date);
	}
	root->add_child("RatingList");

	xmlpp::Node* reel_list = root->add_child ("ReelList");
	
	for (list<shared_ptr<Reel> >::const_iterator i = _reels.begin(); i != _reels.end(); ++i) {
		(*i)->write_to_cpl (reel_list, interop);
	}

	if (crypt) {
		sign (root, crypt->certificates, crypt->signer_key, interop);
	}

	doc.write_to_file_formatted (p.string (), "UTF-8");

	_digest = make_digest (p.string ());
	_length = boost::filesystem::file_size (p.string ());
}

void
CPL::write_to_pkl (xmlpp::Node* node) const
{
	xmlpp::Node* asset = node->add_child ("Asset");
	asset->add_child("Id")->add_child_text ("urn:uuid:" + _id);
	asset->add_child("Hash")->add_child_text (_digest);
	asset->add_child("Size")->add_child_text (lexical_cast<string> (_length));
	asset->add_child("Type")->add_child_text ("text/xml");
}

list<shared_ptr<const Asset> >
CPL::assets () const
{
	list<shared_ptr<const Asset> > a;
	for (list<shared_ptr<Reel> >::const_iterator i = _reels.begin(); i != _reels.end(); ++i) {
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
CPL::write_to_assetmap (xmlpp::Node* node) const
{
	xmlpp::Node* asset = node->add_child ("Asset");
	asset->add_child("Id")->add_child_text ("urn:uuid:" + _id);
	xmlpp::Node* chunk_list = asset->add_child ("ChunkList");
	xmlpp::Node* chunk = chunk_list->add_child ("Chunk");
	chunk->add_child("Path")->add_child_text (_id + "_cpl.xml");
	chunk->add_child("VolumeIndex")->add_child_text ("1");
	chunk->add_child("Offset")->add_child_text("0");
	chunk->add_child("Length")->add_child_text(lexical_cast<string> (_length));
}
	
	
	
bool
CPL::equals (CPL const & other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (_name != other._name && !opt.cpl_names_can_differ) {
		stringstream s;
		s << "names differ: " << _name << " vs " << other._name << "\n";
		note (ERROR, s.str ());
		return false;
	}

	if (_content_kind != other._content_kind) {
		note (ERROR, "content kinds differ");
		return false;
	}

	if (_fps != other._fps) {
		note (ERROR, String::compose ("frames per second differ (%1 vs %2)", _fps, other._fps));
		return false;
	}

	if (_length != other._length) {
		stringstream s;
		note (ERROR, String::compose ("lengths differ (%1 vs %2)", _length, other._length));
	}

	if (_reels.size() != other._reels.size()) {
		note (ERROR, String::compose ("reel counts differ (%1 vs %2)", _reels.size(), other._reels.size()));
		return false;
	}
	
	list<shared_ptr<Reel> >::const_iterator a = _reels.begin ();
	list<shared_ptr<Reel> >::const_iterator b = other._reels.begin ();
	
	while (a != _reels.end ()) {
		if (!(*a)->equals (*b, opt, note)) {
			return false;
		}
		++a;
		++b;
	}

	return true;
}

shared_ptr<xmlpp::Document>
CPL::make_kdm (
	CertificateChain const & certificates,
	string const & signer_key,
	shared_ptr<const Certificate> recipient_cert,
	boost::posix_time::ptime from,
	boost::posix_time::ptime until,
	bool interop,
	MXFMetadata const & mxf_metadata,
	XMLMetadata const & xml_metadata
	) const
{
	assert (recipient_cert);
	
	shared_ptr<xmlpp::Document> doc (new xmlpp::Document);
	xmlpp::Element* root = doc->create_root_node ("DCinemaSecurityMessage");
	root->set_namespace_declaration ("http://www.smpte-ra.org/schemas/430-3/2006/ETM", "");
	root->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "ds");
	root->set_namespace_declaration ("http://www.w3.org/2001/04/xmlenc#", "enc");

	{
		xmlpp::Element* authenticated_public = root->add_child("AuthenticatedPublic");
		authenticated_public->set_attribute("Id", "ID_AuthenticatedPublic");
		xmlAddID (0, doc->cobj(), (const xmlChar *) "ID_AuthenticatedPublic", authenticated_public->get_attribute("Id")->cobj());
		
		authenticated_public->add_child("MessageId")->add_child_text ("urn:uuid:" + make_uuid());
		authenticated_public->add_child("MessageType")->add_child_text ("http://www.smpte-ra.org/430-1/2006/KDM#kdm-key-type");
		authenticated_public->add_child("AnnotationText")->add_child_text (mxf_metadata.product_name);
		authenticated_public->add_child("IssueDate")->add_child_text (xml_metadata.issue_date);

		{
			xmlpp::Element* signer = authenticated_public->add_child("Signer");
			signer->add_child("X509IssuerName", "ds")->add_child_text (recipient_cert->issuer());
			signer->add_child("X509SerialNumber", "ds")->add_child_text (recipient_cert->serial());
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
						serial_element->add_child("X509IssuerName", "ds")->add_child_text (recipient_cert->issuer());
						serial_element->add_child("X509SerialNumber", "ds")->add_child_text (recipient_cert->serial());
					}

					recipient->add_child("X509SubjectName")->add_child_text (recipient_cert->subject());
				}

				kdm_required_extensions->add_child("CompositionPlaylistId")->add_child_text("urn:uuid:" + _id);
				kdm_required_extensions->add_child("ContentTitleText")->add_child_text(_name);
				kdm_required_extensions->add_child("ContentAuthenticator")->add_child_text(certificates.leaf()->thumbprint());
				kdm_required_extensions->add_child("ContentKeysNotValidBefore")->add_child_text("XXX");
				kdm_required_extensions->add_child("ContentKeysNotValidAfter")->add_child_text("XXX");

				{
					xmlpp::Element* authorized_device_info = kdm_required_extensions->add_child("AuthorizedDeviceInfo");
					authorized_device_info->add_child("DeviceListIdentifier")->add_child_text("urn:uuid:" + make_uuid());
					authorized_device_info->add_child("DeviceListDescription")->add_child_text(recipient_cert->subject());
					{
						xmlpp::Element* device_list = authorized_device_info->add_child("DeviceList");
						device_list->add_child("CertificateThumbprint")->add_child_text(recipient_cert->thumbprint());
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

			if (interop) {
				signed_info->add_child("SignatureMethod", "ds")->set_attribute(
					"Algorithm", "http://www.w3.org/2000/09/xmldsig#rsa-sha1"
					);
			} else {
				signed_info->add_child("SignatureMethod", "ds")->set_attribute(
					"Algorithm", "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256"
					);
			}
			
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

/** @return true if we have any encrypted content */
bool
CPL::encrypted () const
{
	for (list<shared_ptr<Reel> >::const_iterator i = _reels.begin(); i != _reels.end(); ++i) {
		if ((*i)->encrypted ()) {
			return true;
		}
	}

	return false;
}

void
CPL::add_kdm (KDM const & kdm)
{
	for (list<shared_ptr<Reel> >::const_iterator i = _reels.begin(); i != _reels.end(); ++i) {
		(*i)->add_kdm (kdm);
	}
}

shared_ptr<parse::AssetMapAsset>
CPL::asset_from_id (list<shared_ptr<const parse::AssetMap> > asset_maps, string id) const
{
	for (list<shared_ptr<const parse::AssetMap> >::const_iterator i = asset_maps.begin(); i != asset_maps.end(); ++i) {
		shared_ptr<parse::AssetMapAsset> a = (*i)->asset_from_id (id);
		if (a) {
			return a;
		}
	}

	return shared_ptr<parse::AssetMapAsset> ();
}
