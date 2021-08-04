/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

    This file is part of libdcp.

    libdcp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libdcp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdcp.  If not, see <http://www.gnu.org/licenses/>.

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/


/** @file  src/cpl.cc
 *  @brief CPL class
 */


#include "certificate_chain.h"
#include "compose.hpp"
#include "cpl.h"
#include "dcp_assert.h"
#include "local_time.h"
#include "metadata.h"
#include "raw_convert.h"
#include "reel.h"
#include "reel_atmos_asset.h"
#include "reel_closed_caption_asset.h"
#include "reel_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_subtitle_asset.h"
#include "util.h"
#include "warnings.h"
#include "xml.h"
LIBDCP_DISABLE_WARNINGS
#include <asdcp/Metadata.h>
LIBDCP_ENABLE_WARNINGS
#include <libxml/parser.h>
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <boost/algorithm/string.hpp>


using std::cout;
using std::dynamic_pointer_cast;
using std::list;
using std::make_pair;
using std::make_shared;
using std::pair;
using std::set;
using std::shared_ptr;
using std::string;
using std::vector;
using boost::optional;
using namespace dcp;


static string const cpl_interop_ns = "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#";
static string const cpl_smpte_ns   = "http://www.smpte-ra.org/schemas/429-7/2006/CPL";
static string const cpl_metadata_ns = "http://www.smpte-ra.org/schemas/429-16/2014/CPL-Metadata";
static string const mca_sub_descriptors_ns = "http://isdcf.com/ns/cplmd/mca";
static string const smpte_395_ns = "http://www.smpte-ra.org/reg/395/2014/13/1/aaf";
static string const smpte_335_ns = "http://www.smpte-ra.org/reg/335/2012";


CPL::CPL (string annotation_text, ContentKind content_kind, Standard standard)
	/* default _content_title_text to annotation_text */
	: _issuer ("libdcp" LIBDCP_VERSION)
	, _creator ("libdcp" LIBDCP_VERSION)
	, _issue_date (LocalTime().as_string())
	, _annotation_text (annotation_text)
	, _content_title_text (annotation_text)
	, _content_kind (content_kind)
	, _standard (standard)
{
	ContentVersion cv;
	cv.label_text = cv.id + LocalTime().as_string();
	_content_versions.push_back (cv);
}


CPL::CPL (boost::filesystem::path file)
	: Asset (file)
	, _content_kind (ContentKind::FEATURE)
{
	cxml::Document f ("CompositionPlaylist");
	f.read_file (file);

	if (f.namespace_uri() == cpl_interop_ns) {
		_standard = Standard::INTEROP;
	} else if (f.namespace_uri() == cpl_smpte_ns) {
		_standard = Standard::SMPTE;
	} else {
		boost::throw_exception (XMLError ("Unrecognised CPL namespace " + f.namespace_uri()));
	}

	_id = remove_urn_uuid (f.string_child ("Id"));
	_annotation_text = f.optional_string_child("AnnotationText");
	_issuer = f.optional_string_child("Issuer").get_value_or("");
	_creator = f.optional_string_child("Creator").get_value_or("");
	_issue_date = f.string_child ("IssueDate");
	_content_title_text = f.string_child ("ContentTitleText");
	_content_kind = content_kind_from_string (f.string_child ("ContentKind"));
	shared_ptr<cxml::Node> content_version = f.optional_node_child ("ContentVersion");
	if (content_version) {
		/* XXX: SMPTE should insist that Id is present */
		_content_versions.push_back (
			ContentVersion (
				content_version->optional_string_child("Id").get_value_or(""),
				content_version->string_child("LabelText")
				)
			);
		content_version->done ();
	} else if (_standard == Standard::SMPTE) {
		/* ContentVersion is required in SMPTE */
		throw XMLError ("Missing ContentVersion tag in CPL");
	}
	auto rating_list = f.node_child ("RatingList");
	for (auto i: rating_list->node_children("Rating")) {
		_ratings.push_back (Rating(i));
	}

	for (auto i: f.node_child("ReelList")->node_children("Reel")) {
		_reels.push_back (make_shared<Reel>(i, _standard));
	}

	auto reel_list = f.node_child ("ReelList");
	auto reels = reel_list->node_children("Reel");
	if (!reels.empty()) {
		auto asset_list = reels.front()->node_child("AssetList");
		auto metadata = asset_list->optional_node_child("CompositionMetadataAsset");
		if (metadata) {
			read_composition_metadata_asset (metadata);
		}
	}

	f.ignore_child ("Issuer");
	f.ignore_child ("Signer");
	f.ignore_child ("Signature");

	f.done ();
}


void
CPL::add (std::shared_ptr<Reel> reel)
{
	_reels.push_back (reel);
}


void
CPL::write_xml (boost::filesystem::path file, shared_ptr<const CertificateChain> signer) const
{
	xmlpp::Document doc;
	xmlpp::Element* root;
	if (_standard == Standard::INTEROP) {
		root = doc.create_root_node ("CompositionPlaylist", cpl_interop_ns);
	} else {
		root = doc.create_root_node ("CompositionPlaylist", cpl_smpte_ns);
	}

	root->add_child("Id")->add_child_text ("urn:uuid:" + _id);
	if (_annotation_text) {
		root->add_child("AnnotationText")->add_child_text (*_annotation_text);
	}
	root->add_child("IssueDate")->add_child_text (_issue_date);
	root->add_child("Issuer")->add_child_text (_issuer);
	root->add_child("Creator")->add_child_text (_creator);
	root->add_child("ContentTitleText")->add_child_text (_content_title_text);
	root->add_child("ContentKind")->add_child_text (content_kind_to_string (_content_kind));
	if (_content_versions.empty()) {
		ContentVersion cv;
		cv.as_xml (root);
	} else {
		_content_versions[0].as_xml (root);
	}

	auto rating_list = root->add_child("RatingList");
	for (auto i: _ratings) {
		i.as_xml (rating_list->add_child("Rating"));
	}

	auto reel_list = root->add_child ("ReelList");

	if (_reels.empty()) {
		throw NoReelsError ();
	}

	bool first = true;
	for (auto i: _reels) {
		auto asset_list = i->write_to_cpl (reel_list, _standard);
		if (first && _standard == Standard::SMPTE) {
			maybe_write_composition_metadata_asset (asset_list);
			first = false;
		}
	}

	indent (root, 0);

	if (signer) {
		signer->sign (root, _standard);
	}

	doc.write_to_file_formatted (file.string(), "UTF-8");

	set_file (file);
}


void
CPL::read_composition_metadata_asset (cxml::ConstNodePtr node)
{
	_cpl_metadata_id = remove_urn_uuid(node->string_child("Id"));

	auto fctt = node->node_child("FullContentTitleText");
	_full_content_title_text = fctt->content();
	_full_content_title_text_language = fctt->optional_string_attribute("language");

	_release_territory = node->optional_string_child("ReleaseTerritory");
	if (_release_territory) {
		_release_territory_scope = node->node_child("ReleaseTerritory")->optional_string_attribute("scope");
	}

	auto vn = node->optional_node_child("VersionNumber");
	if (vn) {
		_version_number = raw_convert<int>(vn->content());
		/* I decided to check for this number being non-negative on being set, and in the verifier, but not here */
		auto vn_status = vn->optional_string_attribute("status");
		if (vn_status) {
			_status = string_to_status (*vn_status);
		}
	}

	_chain = node->optional_string_child("Chain");
	_distributor = node->optional_string_child("Distributor");
	_facility = node->optional_string_child("Facility");

	auto acv = node->optional_node_child("AlternateContentVersionList");
	if (acv) {
		for (auto i: acv->node_children("ContentVersion")) {
			_content_versions.push_back (ContentVersion(i));
		}
	}

	auto lum = node->optional_node_child("Luminance");
	if (lum) {
		_luminance = Luminance (lum);
	}

	_main_sound_configuration = node->optional_string_child("MainSoundConfiguration");

	auto sr = node->optional_string_child("MainSoundSampleRate");
	if (sr) {
		vector<string> sr_bits;
		boost::split (sr_bits, *sr, boost::is_any_of(" "));
		DCP_ASSERT (sr_bits.size() == 2);
		_main_sound_sample_rate = raw_convert<int>(sr_bits[0]);
	}

	_main_picture_stored_area = dcp::Size (
		node->node_child("MainPictureStoredArea")->number_child<int>("Width"),
		node->node_child("MainPictureStoredArea")->number_child<int>("Height")
		);

	_main_picture_active_area = dcp::Size (
		node->node_child("MainPictureActiveArea")->number_child<int>("Width"),
		node->node_child("MainPictureActiveArea")->number_child<int>("Height")
		);

	auto sll = node->optional_string_child("MainSubtitleLanguageList");
	if (sll) {
		vector<string> sll_split;
		boost::split (sll_split, *sll, boost::is_any_of(" "));
		DCP_ASSERT (!sll_split.empty());

		/* If the first language on SubtitleLanguageList is the same as the language of the first subtitle we'll ignore it */
		size_t first = 0;
		if (!_reels.empty()) {
			auto sub = _reels.front()->main_subtitle();
			if (sub) {
				auto lang = sub->language();
				if (lang && lang == sll_split[0]) {
					first = 1;
				}
			}
		}

		for (auto i = first; i < sll_split.size(); ++i) {
			_additional_subtitle_languages.push_back (sll_split[i]);
		}
	}

	auto eml = node->optional_node_child ("ExtensionMetadataList");
	if (eml) {
		for (auto i: eml->node_children("ExtensionMetadata")) {
			auto name = i->optional_string_child("Name");
			if (name && *name == "Sign Language Video") {
				auto property_list = i->node_child("PropertyList");
				for (auto j: property_list->node_children("Property")) {
					auto name = j->optional_string_child("Name");
					auto value = j->optional_string_child("Value");
					if (name && value && *name == "Language Tag") {
						_sign_language_video_language = *value;
					}
				}
			}
		}
	}
}


/** Write a CompositionMetadataAsset node as a child of @param node provided
 *  the required metadata is stored in the object.  If any required metadata
 *  is missing this method will do nothing.
 */
void
CPL::maybe_write_composition_metadata_asset (xmlpp::Element* node) const
{
	if (
		!_main_sound_configuration ||
		!_main_sound_sample_rate ||
		!_main_picture_stored_area ||
		!_main_picture_active_area ||
		_reels.empty() ||
		!_reels.front()->main_picture()) {
		return;
	}

	auto meta = node->add_child("meta:CompositionMetadataAsset");
	meta->set_namespace_declaration (cpl_metadata_ns, "meta");

	meta->add_child("Id")->add_child_text("urn:uuid:" + _cpl_metadata_id);

	auto mp = _reels.front()->main_picture();
	meta->add_child("EditRate")->add_child_text(mp->edit_rate().as_string());
	meta->add_child("IntrinsicDuration")->add_child_text(raw_convert<string>(mp->intrinsic_duration()));

	auto fctt = meta->add_child("FullContentTitleText", "meta");
	if (_full_content_title_text && !_full_content_title_text->empty()) {
		fctt->add_child_text (*_full_content_title_text);
	}
	if (_full_content_title_text_language) {
		fctt->set_attribute("language", *_full_content_title_text_language);
	}

	if (_release_territory) {
		meta->add_child("ReleaseTerritory", "meta")->add_child_text(*_release_territory);
	}

	if (_version_number) {
		xmlpp::Element* vn = meta->add_child("VersionNumber", "meta");
		vn->add_child_text(raw_convert<string>(*_version_number));
		if (_status) {
			vn->set_attribute("status", status_to_string(*_status));
		}
	}

	if (_chain) {
		meta->add_child("Chain", "meta")->add_child_text(*_chain);
	}

	if (_distributor) {
		meta->add_child("Distributor", "meta")->add_child_text(*_distributor);
	}

	if (_facility) {
		meta->add_child("Facility", "meta")->add_child_text(*_facility);
	}

	if (_content_versions.size() > 1) {
		xmlpp::Element* vc = meta->add_child("AlternateContentVersionList", "meta");
		for (size_t i = 1; i < _content_versions.size(); ++i) {
			_content_versions[i].as_xml (vc);
		}
	}

	if (_luminance) {
		_luminance->as_xml (meta, "meta");
	}

	meta->add_child("MainSoundConfiguration", "meta")->add_child_text(*_main_sound_configuration);
	meta->add_child("MainSoundSampleRate", "meta")->add_child_text(raw_convert<string>(*_main_sound_sample_rate) + " 1");

	auto stored = meta->add_child("MainPictureStoredArea", "meta");
	stored->add_child("Width", "meta")->add_child_text(raw_convert<string>(_main_picture_stored_area->width));
	stored->add_child("Height", "meta")->add_child_text(raw_convert<string>(_main_picture_stored_area->height));

	auto active = meta->add_child("MainPictureActiveArea", "meta");
	active->add_child("Width", "meta")->add_child_text(raw_convert<string>(_main_picture_active_area->width));
	active->add_child("Height", "meta")->add_child_text(raw_convert<string>(_main_picture_active_area->height));

	optional<string> first_subtitle_language;
	for (auto i: _reels) {
		if (i->main_subtitle()) {
			first_subtitle_language = i->main_subtitle()->language();
			if (first_subtitle_language) {
				break;
			}
		}
	}

	if (first_subtitle_language || !_additional_subtitle_languages.empty()) {
		string lang;
		if (first_subtitle_language) {
			lang = *first_subtitle_language;
		}
		for (auto const& i: _additional_subtitle_languages) {
			if (!lang.empty()) {
				lang += " ";
			}
			lang += i;
		}
		meta->add_child("MainSubtitleLanguageList", "meta")->add_child_text(lang);
	}

	auto metadata_list = meta->add_child("ExtensionMetadataList", "meta");

	auto add_extension_metadata = [metadata_list](string scope, string name, string property_name, string property_value) {
		auto extension = metadata_list->add_child("ExtensionMetadata", "meta");
		extension->set_attribute("scope", scope);
		extension->add_child("Name", "meta")->add_child_text(name);
		auto property = extension->add_child("PropertyList", "meta")->add_child("Property", "meta");
		property->add_child("Name", "meta")->add_child_text(property_name);
		property->add_child("Value", "meta")->add_child_text(property_value);
	};

	/* SMPTE Bv2.1 8.6.3 */
	add_extension_metadata ("http://isdcf.com/ns/cplmd/app", "Application", "DCP Constraints Profile", "SMPTE-RDD-52:2020-Bv2.1");

	if (_sign_language_video_language) {
		add_extension_metadata ("http://isdcf.com/2017/10/SignLanguageVideo", "Sign Language Video", "Language Tag", *_sign_language_video_language);
	}

	if (_reels.front()->main_sound()) {
		auto asset = _reels.front()->main_sound()->asset();
		if (asset) {
			auto reader = asset->start_read ();
			ASDCP::MXF::SoundfieldGroupLabelSubDescriptor* soundfield;
			ASDCP::Result_t r = reader->reader()->OP1aHeader().GetMDObjectByType(
				asdcp_smpte_dict->ul(ASDCP::MDD_SoundfieldGroupLabelSubDescriptor),
				reinterpret_cast<ASDCP::MXF::InterchangeObject**>(&soundfield)
				);
			if (KM_SUCCESS(r)) {
				auto mca_subs = meta->add_child("mca:MCASubDescriptors");
				mca_subs->set_namespace_declaration (mca_sub_descriptors_ns, "mca");
				mca_subs->set_namespace_declaration (smpte_395_ns, "r0");
				mca_subs->set_namespace_declaration (smpte_335_ns, "r1");
				auto sf = mca_subs->add_child("SoundfieldGroupLabelSubDescriptor", "r0");
				char buffer[64];
				soundfield->InstanceUID.EncodeString(buffer, sizeof(buffer));
				sf->add_child("InstanceID", "r1")->add_child_text("urn:uuid:" + string(buffer));
				soundfield->MCALabelDictionaryID.EncodeString(buffer, sizeof(buffer));
				sf->add_child("MCALabelDictionaryID", "r1")->add_child_text("urn:smpte:ul:" + string(buffer));
				soundfield->MCALinkID.EncodeString(buffer, sizeof(buffer));
				sf->add_child("MCALinkID", "r1")->add_child_text("urn:uuid:" + string(buffer));
				soundfield->MCATagSymbol.EncodeString(buffer, sizeof(buffer));
				sf->add_child("MCATagSymbol", "r1")->add_child_text(buffer);
				if (!soundfield->MCATagName.empty()) {
					soundfield->MCATagName.get().EncodeString(buffer, sizeof(buffer));
					sf->add_child("MCATagName", "r1")->add_child_text(buffer);
				}
				if (!soundfield->RFC5646SpokenLanguage.empty()) {
					soundfield->RFC5646SpokenLanguage.get().EncodeString(buffer, sizeof(buffer));
					sf->add_child("RFC5646SpokenLanguage", "r1")->add_child_text(buffer);
				}

				list<ASDCP::MXF::InterchangeObject*> channels;
				auto r = reader->reader()->OP1aHeader().GetMDObjectsByType(
					asdcp_smpte_dict->ul(ASDCP::MDD_AudioChannelLabelSubDescriptor),
					channels
					);

				for (auto i: channels) {
					auto channel = reinterpret_cast<ASDCP::MXF::AudioChannelLabelSubDescriptor*>(i);
					auto ch = mca_subs->add_child("AudioChannelLabelSubDescriptor", "r0");
					channel->InstanceUID.EncodeString(buffer, sizeof(buffer));
					ch->add_child("InstanceID", "r1")->add_child_text("urn:uuid:" + string(buffer));
					channel->MCALabelDictionaryID.EncodeString(buffer, sizeof(buffer));
					ch->add_child("MCALabelDictionaryID", "r1")->add_child_text("urn:smpte:ul:" + string(buffer));
					channel->MCALinkID.EncodeString(buffer, sizeof(buffer));
					ch->add_child("MCALinkID", "r1")->add_child_text("urn:uuid:" + string(buffer));
					channel->MCATagSymbol.EncodeString(buffer, sizeof(buffer));
					ch->add_child("MCATagSymbol", "r1")->add_child_text(buffer);
					if (!channel->MCATagName.empty()) {
						channel->MCATagName.get().EncodeString(buffer, sizeof(buffer));
						ch->add_child("MCATagName", "r1")->add_child_text(buffer);
					}
					if (!channel->MCAChannelID.empty()) {
						ch->add_child("MCAChannelID", "r1")->add_child_text(raw_convert<string>(channel->MCAChannelID.get()));
					}
					if (!channel->RFC5646SpokenLanguage.empty()) {
						channel->RFC5646SpokenLanguage.get().EncodeString(buffer, sizeof(buffer));
						ch->add_child("RFC5646SpokenLanguage", "r1")->add_child_text(buffer);
					}
					if (!channel->SoundfieldGroupLinkID.empty()) {
						channel->SoundfieldGroupLinkID.get().EncodeString(buffer, sizeof(buffer));
						ch->add_child("SoundfieldGroupLinkID", "r1")->add_child_text("urn:uuid:" + string(buffer));
					}
				}
			}
		}
	}
}


template <class T>
void
add_file_assets (vector<shared_ptr<T>>& assets, vector<shared_ptr<Reel>> reels)
{
	for (auto i: reels) {
		if (i->main_picture ()) {
			assets.push_back (i->main_picture());
		}
		if (i->main_sound ()) {
			assets.push_back (i->main_sound());
		}
		if (i->main_subtitle ()) {
			assets.push_back (i->main_subtitle());
		}
		for (auto j: i->closed_captions()) {
			assets.push_back (j);
		}
		if (i->atmos ()) {
			assets.push_back (i->atmos());
		}
	}
}


vector<shared_ptr<ReelFileAsset>>
CPL::reel_file_assets ()
{
	vector<shared_ptr<ReelFileAsset>> c;
	add_file_assets (c, _reels);
	return c;
}


vector<shared_ptr<const ReelFileAsset>>
CPL::reel_file_assets () const
{
	vector<shared_ptr<const ReelFileAsset>> c;
	add_file_assets (c, _reels);
	return c;
}


bool
CPL::equals (shared_ptr<const Asset> other, EqualityOptions opt, NoteHandler note) const
{
	auto other_cpl = dynamic_pointer_cast<const CPL>(other);
	if (!other_cpl) {
		return false;
	}

	if (_annotation_text != other_cpl->_annotation_text && !opt.cpl_annotation_texts_can_differ) {
		string const s = "CPL: annotation texts differ: " + _annotation_text.get_value_or("") + " vs " + other_cpl->_annotation_text.get_value_or("") + "\n";
		note (NoteType::ERROR, s);
		return false;
	}

	if (_content_kind != other_cpl->_content_kind) {
		note (NoteType::ERROR, "CPL: content kinds differ");
		return false;
	}

	if (_reels.size() != other_cpl->_reels.size()) {
		note (NoteType::ERROR, String::compose ("CPL: reel counts differ (%1 vs %2)", _reels.size(), other_cpl->_reels.size()));
		return false;
	}

	auto a = _reels.begin();
	auto b = other_cpl->_reels.begin();

	while (a != _reels.end ()) {
		if (!(*a)->equals (*b, opt, note)) {
			return false;
		}
		++a;
		++b;
	}

	return true;
}


bool
CPL::any_encrypted () const
{
	for (auto i: _reels) {
		if (i->any_encrypted()) {
			return true;
		}
	}

	return false;
}


bool
CPL::all_encrypted () const
{
	for (auto i: _reels) {
		if (!i->all_encrypted()) {
			return false;
		}
	}

	return true;
}


void
CPL::add (DecryptedKDM const & kdm)
{
	for (auto i: _reels) {
		i->add (kdm);
	}
}

void
CPL::resolve_refs (vector<shared_ptr<Asset>> assets)
{
	for (auto i: _reels) {
		i->resolve_refs (assets);
	}
}

string
CPL::pkl_type (Standard standard) const
{
	return static_pkl_type (standard);
}

string
CPL::static_pkl_type (Standard standard)
{
	switch (standard) {
	case Standard::INTEROP:
		return "text/xml;asdcpKind=CPL";
	case Standard::SMPTE:
		return "text/xml";
	default:
		DCP_ASSERT (false);
	}
}

int64_t
CPL::duration () const
{
	int64_t d = 0;
	for (auto i: _reels) {
		d += i->duration ();
	}
	return d;
}


void
CPL::set_version_number (int v)
{
	if (v < 0) {
		throw BadSettingError ("CPL version number cannot be negative");
	}

	_version_number = v;
}


void
CPL::unset_version_number ()
{
	_version_number = boost::none;
}


void
CPL::set_content_versions (vector<ContentVersion> v)
{
	set<string> ids;
	for (auto i: v) {
		if (!ids.insert(i.id).second) {
			throw DuplicateIdError ("Duplicate ID in ContentVersion list");
		}
	}

	_content_versions = v;
}


optional<ContentVersion>
CPL::content_version () const
{
	if (_content_versions.empty()) {
		return optional<ContentVersion>();
	}

	return _content_versions[0];
}


void
CPL::set_additional_subtitle_languages (vector<dcp::LanguageTag> const& langs)
{
	_additional_subtitle_languages.clear ();
	for (auto const& i: langs) {
		_additional_subtitle_languages.push_back (i.to_string());
	}
}
