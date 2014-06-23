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

#include <libxml/parser.h>
#include "cpl.h"
#include "parse/cpl.h"
#include "util.h"
#include "mono_picture_asset.h"
#include "stereo_picture_asset.h"
#include "sound_asset.h"
#include "subtitle_asset.h"
#include "parse/asset_map.h"
#include "reel.h"
#include "metadata.h"
#include "signer.h"
#include "exceptions.h"
#include "compose.hpp"
#include "raw_convert.h"

using std::string;
using std::stringstream;
using std::ostream;
using std::list;
using std::pair;
using std::make_pair;
using boost::shared_ptr;
using boost::lexical_cast;
using boost::optional;
using namespace libdcp;

CPL::CPL (boost::filesystem::path directory, string name, ContentKind content_kind, int length, int frames_per_second)
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
CPL::CPL (boost::filesystem::path directory, string file, list<PathAssetMap> asset_maps, bool require_mxfs)
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
		boost::throw_exception (FileError ("could not load CPL file", file, e.number ()));
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
				pair<string, shared_ptr<const parse::AssetMapAsset> > asset = asset_from_id (asset_maps, p->id);

				picture.reset (new MonoPictureAsset (asset.first, asset.second->chunks.front()->path));

				picture->read ();
				picture->set_edit_rate (_fps);
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
				pair<string, shared_ptr<const parse::AssetMapAsset> > asset = asset_from_id (asset_maps, p->id);

				picture.reset (new StereoPictureAsset (asset.first, asset.second->chunks.front()->path));

				picture->read ();
				picture->set_edit_rate (_fps);
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
				pair<string, shared_ptr<const parse::AssetMapAsset> > asset = asset_from_id (asset_maps, (*i)->asset_list->main_sound->id);
			
				sound.reset (new SoundAsset (asset.first, asset.second->chunks.front()->path));
				shared_ptr<parse::MainSound> s = (*i)->asset_list->main_sound;

				sound->read ();
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
			
			pair<string, shared_ptr<const parse::AssetMapAsset> > asset = asset_from_id (asset_maps, (*i)->asset_list->main_subtitle->id);

			subtitle.reset (new SubtitleAsset (asset.first, asset.second->chunks.front()->path));

			subtitle->set_entry_point ((*i)->asset_list->main_subtitle->entry_point);
			subtitle->set_edit_rate (_fps);
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

boost::filesystem::path
CPL::filename () const
{
	return _directory / String::compose ("%1_cpl.xml", _id);
}

void
CPL::write_xml (bool interop, XMLMetadata const & metadata, shared_ptr<const Signer> signer) const
{
	boost::filesystem::path p = filename ();

	xmlpp::Document doc;
	xmlpp::Element* root;
	if (interop) {
		root = doc.create_root_node ("CompositionPlaylist", "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#");
	} else {
		root = doc.create_root_node ("CompositionPlaylist", "http://www.smpte-ra.org/schemas/429-7/2006/CPL");
	}

	if (signer) {
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

	xmlpp::Element* reel_list = root->add_child ("ReelList");
	
	for (list<shared_ptr<Reel> >::const_iterator i = _reels.begin(); i != _reels.end(); ++i) {
		(*i)->write_to_cpl (reel_list);
	}

	if (signer) {
		signer->sign (root, interop);
	}

	/* This must not be the _formatted version otherwise signature digests will be wrong */
	doc.write_to_file (p.string (), "UTF-8");

	_digest = make_digest (p.string (), 0);
	_length = boost::filesystem::file_size (p.string ());
}

void
CPL::write_to_pkl (xmlpp::Node* node, bool interop) const
{
	xmlpp::Node* asset = node->add_child ("Asset");
	asset->add_child("Id")->add_child_text ("urn:uuid:" + _id);
	asset->add_child("Hash")->add_child_text (_digest);
	asset->add_child("Size")->add_child_text (raw_convert<string> (_length));
	if (interop) {
		asset->add_child("Type")->add_child_text ("text/xml;asdcpKind=CPL");
	} else {
		asset->add_child("Type")->add_child_text ("text/xml");
	}
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
	chunk->add_child("Length")->add_child_text (raw_convert<string> (_length));
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

pair<string, shared_ptr<const parse::AssetMapAsset> >
CPL::asset_from_id (list<PathAssetMap> asset_maps, string id) const
{
	for (list<PathAssetMap>::const_iterator i = asset_maps.begin(); i != asset_maps.end(); ++i) {
		shared_ptr<parse::AssetMapAsset> a = i->second->asset_from_id (id);
		if (a) {
			return make_pair (i->first, a);
		}
	}

	return make_pair ("", shared_ptr<const parse::AssetMapAsset> ());
}

void
CPL::set_mxf_keys (Key key)
{
	for (list<shared_ptr<Reel> >::const_iterator i = _reels.begin(); i != _reels.end(); ++i) {
		(*i)->set_mxf_keys (key);
	}
}
