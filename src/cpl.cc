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
#include "equality_options.h"
#include "filesystem.h"
#include "local_time.h"
#include "metadata.h"
#include "raw_convert.h"
#include "reel.h"
#include "reel_atmos_asset.h"
#include "reel_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_text_asset.h"
#include "util.h"
#include "verify.h"
#include "version.h"
#include "warnings.h"
#include "xml.h"
LIBDCP_DISABLE_WARNINGS
#include <asdcp/Metadata.h>
LIBDCP_ENABLE_WARNINGS
#include <libxml/parser.h>
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <fmt/format.h>
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


CPL::CPL(string annotation_text, ContentKind content_kind, Standard standard, Profile profile)
	/* default _content_title_text to annotation_text */
	: _issuer("libdcp", dcp::version)
	, _creator("libdcp", dcp::version)
	, _issue_date(LocalTime().as_string())
	, _annotation_text(annotation_text)
	, _content_title_text(annotation_text)
	, _content_kind(content_kind)
	, _standard(standard)
	, _profile(profile)
{
	ContentVersion cv;
	cv.label_text = cv.id + LocalTime().as_string();
	_content_versions.push_back(cv);
}


CPL::CPL(boost::filesystem::path file, vector<dcp::VerificationNote>* notes)
	: Asset(file)
	, _content_kind(ContentKind::FEATURE)
{
	cxml::Document f("CompositionPlaylist");
	f.read_file(dcp::filesystem::fix_long_path(file));

	if (f.namespace_uri() == cpl_interop_ns) {
		_standard = Standard::INTEROP;
	} else if (f.namespace_uri() == cpl_smpte_ns) {
		_standard = Standard::SMPTE;
	} else {
		if (notes) {
			notes->push_back(
				dcp::VerificationNote(
					dcp::VerificationNote::Type::ERROR,
					dcp::VerificationNote::Code::INVALID_CPL_NAMESPACE,
					f.namespace_uri(),
					file
					)
				);
		}
		_standard = Standard::INTEROP;
	}

	_id = remove_urn_uuid(f.string_child("Id"));
	_annotation_text = f.optional_string_child("AnnotationText");
	_issuer = f.optional_string_child("Issuer").get_value_or("");
	_creator = f.optional_string_child("Creator").get_value_or("");
	_issue_date = f.string_child("IssueDate");
	_content_title_text = f.string_child("ContentTitleText");
	auto content_kind = f.node_child("ContentKind");
	_content_kind = ContentKind(content_kind->content(), content_kind->optional_string_attribute("scope"));
	shared_ptr<cxml::Node> content_version = f.optional_node_child("ContentVersion");
	if (content_version) {
		/* XXX: SMPTE should insist that Id is present */
		_content_versions.push_back(
			ContentVersion(
				content_version->optional_string_child("Id").get_value_or(""),
				content_version->string_child("LabelText")
				)
			);
		content_version->done();
	} else if (_standard == Standard::SMPTE) {
		/* ContentVersion is required in SMPTE */
		if (notes) {
			notes->push_back(
				dcp::VerificationNote(
					dcp::VerificationNote::Type::ERROR,
					dcp::VerificationNote::Code::MISSING_CPL_CONTENT_VERSION,
					_id,
					file
					)
				);
		}
	}
	auto rating_list = f.node_child("RatingList");
	for (auto i: rating_list->node_children("Rating")) {
		_ratings.push_back(Rating(i));
	}

	for (auto i: f.node_child("ReelList")->node_children("Reel")) {
		_reels.push_back(make_shared<Reel>(i, _standard));
	}

	auto reel_list = f.node_child("ReelList");
	auto reels = reel_list->node_children("Reel");
	if (!reels.empty()) {
		auto asset_list = reels.front()->node_child("AssetList");
		auto metadata = asset_list->optional_node_child("CompositionMetadataAsset");
		if (metadata) {
			read_composition_metadata_asset(metadata, notes);
			_read_composition_metadata = true;
		} else {
			_profile = Profile::SMPTE_A;
		}
	}

	f.ignore_child("Issuer");
	f.ignore_child("Signer");
	f.ignore_child("Signature");

	f.done();
}


void
CPL::add(std::shared_ptr<Reel> reel)
{
	_reels.push_back(reel);
}


void
CPL::set(std::vector<std::shared_ptr<Reel>> reels)
{
	_reels = reels;
}


void
CPL::write_xml(boost::filesystem::path file, shared_ptr<const CertificateChain> signer) const
{
	xmlpp::Document doc;
	xmlpp::Element* root;
	if (_standard == Standard::INTEROP) {
		root = doc.create_root_node("CompositionPlaylist", cpl_interop_ns);
	} else {
		root = doc.create_root_node("CompositionPlaylist", cpl_smpte_ns);
	}

	cxml::add_text_child(root, "Id", "urn:uuid:" + _id);
	if (_annotation_text) {
		cxml::add_text_child(root, "AnnotationText", *_annotation_text);
	}
	cxml::add_text_child(root, "IssueDate", _issue_date);
	cxml::add_text_child(root, "Issuer", _issuer);
	cxml::add_text_child(root, "Creator", _creator);
	cxml::add_text_child(root, "ContentTitleText", _content_title_text);
	auto content_kind = cxml::add_child(root, "ContentKind");
	content_kind->add_child_text(_content_kind.name());
	if (_content_kind.scope()) {
		content_kind->set_attribute("scope", *_content_kind.scope());
	}
	if (_content_versions.empty()) {
		ContentVersion cv;
		cv.as_xml(root);
	} else {
		_content_versions[0].as_xml(root);
	}

	auto rating_list = cxml::add_child(root, "RatingList");
	for (auto i: _ratings) {
		i.as_xml(cxml::add_child(rating_list, "Rating"));
	}

	auto reel_list = cxml::add_child(root, "ReelList");

	if (_reels.empty()) {
		throw NoReelsError();
	}

	bool first = true;
	for (auto i: _reels) {
		auto asset_list = i->write_to_cpl(reel_list, _standard);
		if (first && _standard == Standard::SMPTE && _profile != Profile::SMPTE_A) {
			maybe_write_composition_metadata_asset(asset_list);
			first = false;
		}
	}

	indent(root, 0);

	if (signer) {
		signer->sign(root, _standard);
	}

	doc.write_to_file_formatted(dcp::filesystem::fix_long_path(file).string(), "UTF-8");

	set_file(file);
}


void
CPL::read_composition_metadata_asset(cxml::ConstNodePtr node, vector<dcp::VerificationNote>* notes)
{
	_cpl_metadata_id = remove_urn_uuid(node->string_child("Id"));

	/* FullContentTitleText is compulsory but in DoM #2295 we saw a commercial tool which
	 * apparently didn't include it, so as usual we have to be defensive.
	 */
	if (auto fctt = node->optional_node_child("FullContentTitleText")) {
		_full_content_title_text = fctt->content();
		_full_content_title_text_language = fctt->optional_string_attribute("language");
	}

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
			_status = string_to_status(*vn_status);
		}
	}

	_chain = node->optional_string_child("Chain");
	_distributor = node->optional_string_child("Distributor");
	_facility = node->optional_string_child("Facility");

	auto acv = node->optional_node_child("AlternateContentVersionList");
	if (acv) {
		for (auto i: acv->node_children("ContentVersion")) {
			_content_versions.push_back(ContentVersion(i));
		}
	}

	auto lum = node->optional_node_child("Luminance");
	if (lum) {
		_luminance = Luminance(lum);
	}

	if (auto msc = node->optional_string_child("MainSoundConfiguration")) {
		_main_sound_configuration = MainSoundConfiguration(*msc);
		if (!_main_sound_configuration->valid() && _standard == dcp::Standard::SMPTE && notes) {
			/* With Interop DCPs this node may not make any sense, but that's OK */
			notes->push_back(
				dcp::VerificationNote(
					dcp::VerificationNote::Type::ERROR,
					dcp::VerificationNote::Code::INVALID_MAIN_SOUND_CONFIGURATION,
					fmt::format("{} could not be parsed", _main_sound_configuration->as_string()),
					*_file
					).set_cpl_id(_id)
				);
		}
	}

	auto sr = node->optional_string_child("MainSoundSampleRate");
	if (sr) {
		vector<string> sr_bits;
		boost::split(sr_bits, *sr, boost::is_any_of(" "));
		DCP_ASSERT(sr_bits.size() == 2);
		_main_sound_sample_rate = raw_convert<int>(sr_bits[0]);
	}

	if (_standard == dcp::Standard::SMPTE) {
		_main_picture_stored_area = dcp::Size(
			node->node_child("MainPictureStoredArea")->number_child<int>("Width"),
			node->node_child("MainPictureStoredArea")->number_child<int>("Height")
			);
	}

	_main_picture_active_area = dcp::Size(
		node->node_child("MainPictureActiveArea")->number_child<int>("Width"),
		node->node_child("MainPictureActiveArea")->number_child<int>("Height")
		);

	auto sll = node->optional_string_child("MainSubtitleLanguageList");
	if (sll) {
		vector<string> sll_split;
		boost::split(sll_split, *sll, boost::is_any_of(" "));
		DCP_ASSERT(!sll_split.empty());

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
			_additional_subtitle_languages.push_back(sll_split[i]);
		}
	}

	auto eml = node->optional_node_child("ExtensionMetadataList");

	auto extension_metadata = [eml](string scope, string name, string property) -> boost::optional<std::string> {
		if (!eml) {
			return {};
		}

		for (auto i: eml->node_children("ExtensionMetadata")) {
			auto xml_scope = i->optional_string_attribute("scope");
			auto xml_name = i->optional_string_child("Name");
			if (xml_scope && *xml_scope == scope && xml_name && *xml_name == name) {
				auto property_list = i->node_child("PropertyList");
				for (auto j: property_list->node_children("Property")) {
					auto property_name = j->optional_string_child("Name");
					auto property_value = j->optional_string_child("Value");
					if (property_name && property_value && *property_name == property) {
						return property_value;
					}
				}
			}
		}

		return {};
	};

	_sign_language_video_language = extension_metadata("http://isdcf.com/2017/10/SignLanguageVideo", "Sign Language Video", "Language Tag");
	_dolby_edr_image_transfer_function = extension_metadata("http://www.dolby.com/schemas/2014/EDR-Metadata", "Dolby EDR", "image transfer function");

	if (node->optional_node_child("MCASubDescriptors")) {
		_profile = Profile::SMPTE_BV21;
	} else {
		_profile = Profile::SMPTE_BV20;
	}
}


void
CPL::write_mca_subdescriptors(xmlpp::Element* parent, shared_ptr<const SoundAsset> asset) const
{
	auto reader = asset->start_read();
	ASDCP::MXF::SoundfieldGroupLabelSubDescriptor* soundfield;
	ASDCP::Result_t r = reader->reader()->OP1aHeader().GetMDObjectByType(
		asdcp_smpte_dict->ul(ASDCP::MDD_SoundfieldGroupLabelSubDescriptor),
		reinterpret_cast<ASDCP::MXF::InterchangeObject**>(&soundfield)
		);
	if (KM_SUCCESS(r)) {
		auto mca_subs = cxml::add_child(parent, "mca:MCASubDescriptors");
		mca_subs->set_namespace_declaration(mca_sub_descriptors_ns, "mca");
		mca_subs->set_namespace_declaration(smpte_395_ns, "r0");
		mca_subs->set_namespace_declaration(smpte_335_ns, "r1");
		auto sf = cxml::add_child(mca_subs, "SoundfieldGroupLabelSubDescriptor", string("r0"));
		char buffer[64];
		soundfield->InstanceUID.EncodeString(buffer, sizeof(buffer));
		cxml::add_child(sf, "InstanceID", string("r1"))->add_child_text("urn:uuid:" + string(buffer));
		soundfield->MCALabelDictionaryID.EncodeString(buffer, sizeof(buffer));
		cxml::add_child(sf, "MCALabelDictionaryID", string("r1"))->add_child_text("urn:smpte:ul:" + string(buffer));
		soundfield->MCALinkID.EncodeString(buffer, sizeof(buffer));
		cxml::add_child(sf, "MCALinkID", string("r1"))->add_child_text("urn:uuid:" + string(buffer));
		soundfield->MCATagSymbol.EncodeString(buffer, sizeof(buffer));
		cxml::add_child(sf, "MCATagSymbol", string("r1"))->add_child_text(buffer);
		if (!soundfield->MCATagName.empty()) {
			soundfield->MCATagName.get().EncodeString(buffer, sizeof(buffer));
			cxml::add_child(sf, "MCATagName", string("r1"))->add_child_text(buffer);
		}
		if (!soundfield->RFC5646SpokenLanguage.empty()) {
			soundfield->RFC5646SpokenLanguage.get().EncodeString(buffer, sizeof(buffer));
			cxml::add_child(sf, "RFC5646SpokenLanguage", string("r1"))->add_child_text(buffer);
		}

		/* Find the MCA subdescriptors in the MXF so that we can also write them here */
		list<ASDCP::MXF::InterchangeObject*> channels;
		auto r = reader->reader()->OP1aHeader().GetMDObjectsByType(
			asdcp_smpte_dict->ul(ASDCP::MDD_AudioChannelLabelSubDescriptor),
			channels
			);

		for (auto i: channels) {
			auto channel = reinterpret_cast<ASDCP::MXF::AudioChannelLabelSubDescriptor*>(i);
			auto ch = cxml::add_child(mca_subs, "AudioChannelLabelSubDescriptor", string("r0"));
			channel->InstanceUID.EncodeString(buffer, sizeof(buffer));
			cxml::add_child(ch, "InstanceID", string("r1"))->add_child_text("urn:uuid:" + string(buffer));
			channel->MCALabelDictionaryID.EncodeString(buffer, sizeof(buffer));
			cxml::add_child(ch, "MCALabelDictionaryID", string("r1"))->add_child_text("urn:smpte:ul:" + string(buffer));
			channel->MCALinkID.EncodeString(buffer, sizeof(buffer));
			cxml::add_child(ch, "MCALinkID", string("r1"))->add_child_text("urn:uuid:" + string(buffer));
			channel->MCATagSymbol.EncodeString(buffer, sizeof(buffer));
			cxml::add_child(ch, "MCATagSymbol", string("r1"))->add_child_text(buffer);
			if (!channel->MCATagName.empty()) {
				channel->MCATagName.get().EncodeString(buffer, sizeof(buffer));
				cxml::add_child(ch, "MCATagName", string("r1"))->add_child_text(buffer);
			}
			if (!channel->MCAChannelID.empty()) {
				cxml::add_child(ch, "MCAChannelID", string("r1"))->add_child_text(fmt::to_string(channel->MCAChannelID.get()));
			}
			if (!channel->RFC5646SpokenLanguage.empty()) {
				channel->RFC5646SpokenLanguage.get().EncodeString(buffer, sizeof(buffer));
				cxml::add_child(ch, "RFC5646SpokenLanguage", string("r1"))->add_child_text(buffer);
			}
			if (!channel->SoundfieldGroupLinkID.empty()) {
				channel->SoundfieldGroupLinkID.get().EncodeString(buffer, sizeof(buffer));
				cxml::add_child(ch, "SoundfieldGroupLinkID", string("r1"))->add_child_text("urn:uuid:" + string(buffer));
			}
		}
	}
}


/** Write a CompositionMetadataAsset node as a child of @param node provided
 *  the required metadata is stored in the object.  If any required metadata
 *  is missing this method will do nothing.
 */
void
CPL::maybe_write_composition_metadata_asset(xmlpp::Element* node) const
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

	auto meta = cxml::add_child(node, "meta:CompositionMetadataAsset");
	meta->set_namespace_declaration(cpl_metadata_ns, "meta");

	cxml::add_text_child(meta, "Id", "urn:uuid:" + _cpl_metadata_id);

	auto mp = _reels.front()->main_picture();
	cxml::add_text_child(meta, "EditRate", mp->edit_rate().as_string());
	cxml::add_text_child(meta, "IntrinsicDuration", fmt::to_string(mp->intrinsic_duration()));

	auto fctt = cxml::add_child(meta, "FullContentTitleText", string("meta"));
	if (_full_content_title_text && !_full_content_title_text->empty()) {
		fctt->add_child_text(*_full_content_title_text);
	}
	if (_full_content_title_text_language) {
		fctt->set_attribute("language", *_full_content_title_text_language);
	}

	if (_release_territory) {
		cxml::add_child(meta, "ReleaseTerritory", string("meta"))->add_child_text(*_release_territory);
	}

	if (_version_number) {
		auto vn = cxml::add_child(meta, "VersionNumber", string("meta"));
		vn->add_child_text(fmt::to_string(*_version_number));
		if (_status) {
			vn->set_attribute("status", status_to_string(*_status));
		}
	}

	if (_chain) {
		cxml::add_child(meta, "Chain", string("meta"))->add_child_text(*_chain);
	}

	if (_distributor) {
		cxml::add_child(meta, "Distributor", string("meta"))->add_child_text(*_distributor);
	}

	if (_facility) {
		cxml::add_child(meta, "Facility", string("meta"))->add_child_text(*_facility);
	}

	if (_content_versions.size() > 1) {
		auto vc = cxml::add_child(meta, "AlternateContentVersionList", string("meta"));
		for (size_t i = 1; i < _content_versions.size(); ++i) {
			_content_versions[i].as_xml(vc);
		}
	}

	if (_luminance) {
		_luminance->as_xml(meta, "meta");
	}

	if (_main_sound_configuration) {
		cxml::add_child(meta, "MainSoundConfiguration", string("meta"))->add_child_text(_main_sound_configuration->as_string());
	}
	cxml::add_child(meta, "MainSoundSampleRate", string("meta"))->add_child_text(fmt::to_string(*_main_sound_sample_rate) + " 1");

	auto stored = cxml::add_child(meta, "MainPictureStoredArea", string("meta"));
	cxml::add_child(stored, "Width", string("meta"))->add_child_text(fmt::to_string(_main_picture_stored_area->width));
	cxml::add_child(stored, "Height", string("meta"))->add_child_text(fmt::to_string(_main_picture_stored_area->height));

	auto active = cxml::add_child(meta, "MainPictureActiveArea", string("meta"));
	cxml::add_child(active, "Width", string("meta"))->add_child_text(fmt::to_string(_main_picture_active_area->width));
	cxml::add_child(active, "Height", string("meta"))->add_child_text(fmt::to_string(_main_picture_active_area->height));

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
		cxml::add_child(meta, "MainSubtitleLanguageList", string("meta"))->add_child_text(lang);
	}

	auto metadata_list = cxml::add_child(meta, "ExtensionMetadataList", string("meta"));

	auto add_extension_metadata = [metadata_list](string scope, string name, string property_name, string property_value) {
		auto extension = cxml::add_child(metadata_list, "ExtensionMetadata", string("meta"));
		extension->set_attribute("scope", scope);
		cxml::add_child(extension, "Name", string("meta"))->add_child_text(name);
		auto property = cxml::add_child(cxml::add_child(extension, "PropertyList", string("meta")), "Property", string("meta"));
		cxml::add_child(property, "Name", string("meta"))->add_child_text(property_name);
		cxml::add_child(property, "Value", string("meta"))->add_child_text(property_value);
	};

	string const profile_name = _profile == Profile::SMPTE_BV20 ? "2.0" : "2.1";
	/* SMPTE Bv2.1 8.6.3 */
	add_extension_metadata("http://isdcf.com/ns/cplmd/app", "Application", "DCP Constraints Profile", fmt::format("SMPTE-RDD-52:2020-Bv{}", profile_name));

	if (_sign_language_video_language) {
		add_extension_metadata("http://isdcf.com/2017/10/SignLanguageVideo", "Sign Language Video", "Language Tag", *_sign_language_video_language);
	}

	if (_dolby_edr_image_transfer_function) {
		add_extension_metadata("http://www.dolby.com/schemas/2014/EDR-Metadata", "Dolby EDR", "image transfer function", *_dolby_edr_image_transfer_function);
	}

	if (_reels.front()->main_sound()) {
		auto asset = _reels.front()->main_sound()->asset();
		if (asset && _profile == Profile::SMPTE_BV21) {
			write_mca_subdescriptors(meta, asset);
		}
	}
}


template <class T>
void
add_file_assets(vector<shared_ptr<T>>& assets, vector<shared_ptr<Reel>> reels)
{
	for (auto i: reels) {
		if (i->main_picture()) {
			assets.push_back(i->main_picture());
		}
		if (i->main_sound()) {
			assets.push_back(i->main_sound());
		}
		if (i->main_subtitle()) {
			assets.push_back(i->main_subtitle());
		}
		if (i->main_caption()) {
			assets.push_back(i->main_caption());
		}
		for (auto j: i->closed_subtitles()) {
			assets.push_back(j);
		}
		for (auto j: i->closed_captions()) {
			assets.push_back(j);
		}
		if (i->atmos()) {
			assets.push_back(i->atmos());
		}
	}
}


vector<shared_ptr<ReelFileAsset>>
CPL::reel_file_assets()
{
	vector<shared_ptr<ReelFileAsset>> c;
	add_file_assets(c, _reels);
	return c;
}


vector<shared_ptr<const ReelFileAsset>>
CPL::reel_file_assets() const
{
	vector<shared_ptr<const ReelFileAsset>> c;
	add_file_assets(c, _reels);
	return c;
}


bool
CPL::equals(shared_ptr<const Asset> other, EqualityOptions const& opt, NoteHandler note) const
{
	auto other_cpl = dynamic_pointer_cast<const CPL>(other);
	if (!other_cpl) {
		return false;
	}

	if (_annotation_text != other_cpl->_annotation_text && !opt.cpl_annotation_texts_can_differ) {
		string const s = "CPL: annotation texts differ: " + _annotation_text.get_value_or("") + " vs " + other_cpl->_annotation_text.get_value_or("") + "\n";
		note(NoteType::ERROR, s);
		return false;
	}

	if (_content_kind != other_cpl->_content_kind) {
		note(NoteType::ERROR, "CPL: content kinds differ");
		return false;
	}

	if (_reels.size() != other_cpl->_reels.size()) {
		note(NoteType::ERROR, String::compose("CPL: reel counts differ (%1 vs %2)", _reels.size(), other_cpl->_reels.size()));
		return false;
	}

	auto a = _reels.begin();
	auto b = other_cpl->_reels.begin();

	while (a != _reels.end()) {
		if (!(*a)->equals(*b, opt, note)) {
			return false;
		}
		++a;
		++b;
	}

	return true;
}


bool
CPL::any_encrypted() const
{
	for (auto i: _reels) {
		if (i->any_encrypted()) {
			return true;
		}
	}

	return false;
}


bool
CPL::all_encrypted() const
{
	for (auto i: _reels) {
		if (!i->all_encrypted()) {
			return false;
		}
	}

	return true;
}


void
CPL::add(DecryptedKDM const & kdm)
{
	for (auto i: _reels) {
		i->add(kdm);
	}
}

void
CPL::resolve_refs(vector<shared_ptr<Asset>> assets)
{
	for (auto i: _reels) {
		i->resolve_refs(assets);
	}
}

string
CPL::pkl_type(Standard standard) const
{
	return static_pkl_type(standard);
}

string
CPL::static_pkl_type(Standard standard)
{
	switch (standard) {
	case Standard::INTEROP:
		return "text/xml;asdcpKind=CPL";
	case Standard::SMPTE:
		return "text/xml";
	default:
		DCP_ASSERT(false);
	}
}

int64_t
CPL::duration() const
{
	int64_t d = 0;
	for (auto i: _reels) {
		d += i->duration();
	}
	return d;
}


void
CPL::set_version_number(int v)
{
	if (v < 0) {
		throw BadSettingError("CPL version number cannot be negative");
	}

	_version_number = v;
}


void
CPL::unset_version_number()
{
	_version_number = boost::none;
}


void
CPL::set_content_versions(vector<ContentVersion> v)
{
	std::set<string> ids;
	for (auto i: v) {
		if (!ids.insert(i.id).second) {
			throw DuplicateIdError("Duplicate ID in ContentVersion list");
		}
	}

	_content_versions = v;
}


optional<ContentVersion>
CPL::content_version() const
{
	if (_content_versions.empty()) {
		return optional<ContentVersion>();
	}

	return _content_versions[0];
}


void
CPL::set_additional_subtitle_languages(vector<dcp::LanguageTag> const& langs)
{
	_additional_subtitle_languages.clear();
	for (auto const& i: langs) {
		_additional_subtitle_languages.push_back(i.as_string());
	}
}


void
CPL::set_main_picture_active_area(dcp::Size area)
{
	if (area.width % 2) {
		throw BadSettingError("Main picture active area width is not a multiple of 2");
	}

	if (area.height % 2) {
		throw BadSettingError("Main picture active area height is not a multiple of 2");
	}

	_main_picture_active_area = area;
}

