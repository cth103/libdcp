/*
    Copyright (C) 2014-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/cpl.h
 *  @brief CPL class
 */


#ifndef LIBDCP_CPL_H
#define LIBDCP_CPL_H


#include "asset.h"
#include "certificate.h"
#include "key.h"
#include "language_tag.h"
#include "types.h"
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <memory>
#include <vector>


struct verify_invalid_language3;


namespace dcp {


class ReelFileAsset;
class Reel;
class MXFMetadata;
class CertificateChain;
class DecryptedKDM;


/** @class CPL
 *  @brief A Composition Playlist
 */
class CPL : public Asset
{
public:
	CPL (std::string annotation_text, ContentKind content_kind, Standard standard);

	/** Construct a CPL object from a XML file */
	explicit CPL (boost::filesystem::path file);

	bool equals (
		std::shared_ptr<const Asset> other,
		EqualityOptions options,
		NoteHandler note
		) const override;

	/** Add a reel to this CPL
	 *  @param reel Reel to add
	 */
	void add (std::shared_ptr<Reel> reel);

	/** Add a KDM to this CPL.  If the KDM is for any of this CPLs assets it will be used
	 *  to decrypt those assets.
	 *  @param kdm KDM.
	 */
	void add (DecryptedKDM const &);

	/** @return the reels in this CPL */
	std::vector<std::shared_ptr<Reel>> reels () const {
		return _reels;
	}

	/** @return the ReelFileAssets in this CPL in all reels */
	std::vector<std::shared_ptr<const ReelFileAsset>> reel_file_assets () const;
	std::vector<std::shared_ptr<ReelFileAsset>> reel_file_assets ();

	/** @return true if we have any encrypted content */
	bool any_encrypted () const;

	/** @return true if we have all our encryptable content is encrypted */
	bool all_encrypted () const;

	/** Write an CompositonPlaylist XML file
	 *
	 *  @param file Filename to write
	 *  @param signer Signer to sign the CPL, or 0 to add no signature
	 */
	void write_xml (
		boost::filesystem::path file,
		std::shared_ptr<const CertificateChain>
		) const;

	void resolve_refs (std::vector<std::shared_ptr<Asset>>);

	int64_t duration () const;

	std::string issuer () const {
		return _issuer;
	}

	void set_issuer (std::string issuer) {
		_issuer = issuer;
	}

	std::string creator () const {
		return _creator;
	}

	void set_creator (std::string creator) {
		_creator = creator;
	}

	void set_issue_date (std::string issue_date) {
		_issue_date = issue_date;
	}

	/** @return contents of the &lt;AnnotationText&gt; node, if present */
	boost::optional<std::string> annotation_text () const {
		return _annotation_text;
	}

	void set_annotation_text (std::string at) {
		_annotation_text = at;
	}

	/** @return contents of the &lt;ContentTitleText&gt; node */
	std::string content_title_text () const {
		return _content_title_text;
	}

	void set_content_title_text (std::string ct) {
		_content_title_text = ct;
	}

	void set_content_kind (dcp::ContentKind k) {
		_content_kind = k;
	}

	/** @return the type of the content, used by media servers
	 *  to categorise things (e.g. feature, trailer, etc.)
	 */
	ContentKind content_kind () const {
		return _content_kind;
	}

	boost::optional<ContentVersion> content_version () const;

	std::vector<ContentVersion> content_versions () const {
		return _content_versions;
	}

	void set_content_version (ContentVersion v) {
		_content_versions.clear ();
		_content_versions.push_back (v);
	}

	void set_content_versions (std::vector<ContentVersion> v);

	std::vector<Rating> ratings () const {
		return _ratings;
	}

	void set_ratings (std::vector<Rating> r) {
		_ratings = r;
	}

	boost::optional<std::string> full_content_title_text () const {
		return _full_content_title_text;
	}

	void set_full_content_title_text (std::string t) {
		_full_content_title_text = t;
	}

	boost::optional<std::string> full_content_title_text_language () const {
		return _full_content_title_text_language;
	}

	void set_full_content_title_text_language (dcp::LanguageTag l) {
		_full_content_title_text_language = l.to_string();
	}

	boost::optional<std::string> release_territory () const {
		return _release_territory;
	}

	void set_release_territory (dcp::LanguageTag::RegionSubtag t) {
		_release_territory = t.subtag();
	}

	boost::optional<std::string> release_territory_scope () const {
		return _release_territory_scope;
	}

	boost::optional<int> version_number () const {
		return _version_number;
	}

	void set_version_number (int v);

	void unset_version_number ();

	boost::optional<Status> status () const {
		return _status;
	}

	void set_status (Status s) {
		_status = s;
	}

	boost::optional<std::string> chain () const {
		return _chain;
	}

	void set_chain (std::string c) {
		_chain = c;
	}

	boost::optional<std::string> distributor () const {
		return _distributor;
	}

	void set_distributor (std::string d) {
		_distributor = d;
	}

	boost::optional<std::string> facility () const {
		return _facility;
	}

	void set_facility (std::string f) {
		_facility = f;
	}

	boost::optional<Luminance> luminance () const {
		return _luminance;
	}

	void set_luminance (Luminance l) {
		_luminance = l;
	}

	boost::optional<std::string> main_sound_configuration () const {
		return _main_sound_configuration;
	}

	void set_main_sound_configuration (std::string c) {
		_main_sound_configuration = c;
	}

	boost::optional<int> main_sound_sample_rate () const {
		return _main_sound_sample_rate;
	}

	void set_main_sound_sample_rate (int r) {
		_main_sound_sample_rate = r;
	}

	boost::optional<dcp::Size> main_picture_stored_area () const {
		return _main_picture_stored_area;
	}

	void set_main_picture_stored_area (dcp::Size s) {
		_main_picture_stored_area = s;
	}

	boost::optional<dcp::Size> main_picture_active_area () const {
		return _main_picture_active_area;
	}

	void set_main_picture_active_area (dcp::Size s) {
		_main_picture_active_area = s;
	}

	std::vector<std::string> additional_subtitle_languages () const {
		return _additional_subtitle_languages;
	}

	void set_additional_subtitle_languages (std::vector<dcp::LanguageTag> const& lang);

	Standard standard () const {
		return _standard;
	}

	static std::string static_pkl_type (Standard standard);

protected:
	/** @return type string for PKLs for this asset */
	std::string pkl_type (Standard standard) const override;

private:
	friend struct ::verify_invalid_language3;

	void maybe_write_composition_metadata_asset (xmlpp::Element* node) const;
	void read_composition_metadata_asset (cxml::ConstNodePtr node);

	std::string _issuer;
	std::string _creator;
	std::string _issue_date;
	boost::optional<std::string> _annotation_text;
	std::string _content_title_text;            ///< &lt;ContentTitleText&gt;
	ContentKind _content_kind;                  ///< &lt;ContentKind&gt;
	std::vector<ContentVersion> _content_versions;
	std::vector<Rating> _ratings;
	/** ID for CompositionMetadataAsset tag; either a random one, ready for writing a new tag,
	 *  or the one read in from the existing CPL.
	 */
	std::string _cpl_metadata_id = make_uuid();
	/** Human-readable name of the composition, without any metadata (i.e. no -FTR-EN-XX- etc.) */
	boost::optional<std::string> _full_content_title_text;
	boost::optional<std::string> _full_content_title_text_language;
	/** This is stored and returned as a string so that we can tolerate non-RFC-5646 strings,
	 *  but must be set as a dcp::LanguageTag to try to ensure that we create compliant output.
	 */
	boost::optional<std::string> _release_territory;
	boost::optional<std::string> _release_territory_scope;
	boost::optional<int> _version_number;
	boost::optional<Status> _status;
	boost::optional<std::string> _chain;
	boost::optional<std::string> _distributor;
	boost::optional<std::string> _facility;
	boost::optional<Luminance> _luminance;
	boost::optional<std::string> _main_sound_configuration;
	boost::optional<int> _main_sound_sample_rate;
	boost::optional<dcp::Size> _main_picture_stored_area;
	boost::optional<dcp::Size> _main_picture_active_area;
	/* See note for _release_territory above */
	std::vector<std::string> _additional_subtitle_languages;

	std::vector<std::shared_ptr<Reel>> _reels;

	/** Standard of CPL that was read in */
	Standard _standard;
};


}


#endif
