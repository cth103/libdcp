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


#ifndef LIBDCP_SMPTE_TEXT_ASSET_H
#define LIBDCP_SMPTE_TEXT_ASSET_H


/** @file  src/smpte_text_asset.h
 *  @brief SMPTETextAsset class
 */


#include "crypto_context.h"
#include "language_tag.h"
#include "local_time.h"
#include "mxf.h"
#include "text_asset.h"
#include "subtitle_standard.h"
#include <boost/filesystem.hpp>


namespace ASDCP {
	namespace TimedText {
		class MXFReader;
	}
}


struct verify_invalid_language1;
struct verify_invalid_language2;
struct write_subtitles_in_vertical_order_with_top_alignment;
struct write_subtitles_in_vertical_order_with_bottom_alignment;


namespace dcp {


class SMPTELoadFontNode;


/** @class SMPTETextAsset
 *  @brief A set of subtitles/captions to be read and/or written in the SMPTE format
 */
class SMPTETextAsset : public TextAsset, public MXF
{
public:
	explicit SMPTETextAsset(SubtitleStandard standard = SubtitleStandard::SMPTE_2014);

	/** Construct a SMPTETextAsset by reading an MXF or XML file
	 *  @param file Filename
	 */
	explicit SMPTETextAsset(boost::filesystem::path file);

	bool equals (
		std::shared_ptr<const Asset>,
		EqualityOptions const&,
		NoteHandler note
		) const override;

	std::vector<std::shared_ptr<LoadFontNode>> load_font_nodes () const override;

	std::string xml_as_string () const override;

	/** Write this content to a MXF file */
	void write (boost::filesystem::path path) const override;

	void add(std::shared_ptr<Text>) override;
	void add_font (std::string id, dcp::ArrayData data) override;
	void set_key (Key key) override;

	void set_content_title_text (std::string t) {
		_content_title_text = t;
	}

	void set_language (dcp::LanguageTag l) {
		_language = l.as_string();
	}

	void set_issue_date (LocalTime t) {
		_issue_date = t;
	}

	void set_reel_number (int r) {
		_reel_number = r;
	}

	void set_edit_rate (Fraction e) {
		_edit_rate = e;
	}

	void set_time_code_rate (int t) {
		_time_code_rate = t;
	}

	void set_start_time (Time t) {
		_start_time = t;
	}

	void set_intrinsic_duration (int64_t d) {
		_intrinsic_duration = d;
	}

	int64_t intrinsic_duration () const {
		return _intrinsic_duration;
	}

	/** @return title of the film that these subtitles/captions are for,
	 *  to be presented to the user
	 */
	std::string content_title_text () const {
		return _content_title_text;
	}

	/** @return Language, if one was set.  This should be a xs:language, but
	 *  it might not be if a non-compliant DCP was read in.
	 */
	boost::optional<std::string> language () const {
		return _language;
	}

	/** @return annotation text, to be presented to the user */
	boost::optional<std::string> annotation_text () const {
		return _annotation_text;
	}

	/** @return file issue time and date */
	LocalTime issue_date () const {
		return _issue_date;
	}

	boost::optional<int> reel_number () const {
		return _reel_number;
	}

	Fraction edit_rate () const {
		return _edit_rate;
	}

	/** @return subdivision of 1 second that is used for text times;
	 *  e.g. a time_code_rate of 250 means that a text time of 0:0:0:001
	 *  represents 4ms.
	 */
	int time_code_rate () const override {
		return _time_code_rate;
	}

	boost::optional<Time> start_time () const {
		return _start_time;
	}

	/** @return ID from XML's <Id> tag, or the <Id> that will be used when writing the XML,
	 *  or boost::none if this content is encrypted and no key is available.
	 */
	boost::optional<std::string> xml_id () const {
		return _xml_id;
	}

	/** @return ResourceID read from any MXF that was read */
	boost::optional<std::string> resource_id () const {
		return _resource_id;
	}

	SubtitleStandard subtitle_standard() const override {
		return _subtitle_standard;
	}

	static bool valid_mxf (boost::filesystem::path);
	static std::string static_pkl_type (Standard) {
		return "application/mxf";
	}

protected:

	std::string pkl_type (Standard s) const override {
		return static_pkl_type (s);
	}

private:
	friend struct ::write_smpte_subtitle_test;
	friend struct ::write_smpte_subtitle_test2;
	friend struct ::verify_invalid_language1;
	friend struct ::verify_invalid_language2;
	friend struct ::write_subtitles_in_vertical_order_with_top_alignment;
	friend struct ::write_subtitles_in_vertical_order_with_bottom_alignment;

	void read_fonts (std::shared_ptr<ASDCP::TimedText::MXFReader>);
	void parse_xml (std::shared_ptr<cxml::Document> xml);
	void read_mxf_descriptor (std::shared_ptr<ASDCP::TimedText::MXFReader> reader);
	void read_mxf_resources (std::shared_ptr<ASDCP::TimedText::MXFReader> reader, std::shared_ptr<DecryptionContext> dec);
	std::string schema_namespace() const;

	/** The total length of this content in video frames.  The amount of
	 *  content presented may be less than this.
	 */
	int64_t _intrinsic_duration = 0;
	/** <ContentTitleText> from the asset */
	std::string _content_title_text;
	/** This is stored and returned as a string so that we can tolerate non-RFC-5646 strings,
	 *  but must be set as a dcp::LanguageTag to try to ensure that we create compliant output.
	 */
	boost::optional<std::string> _language;
	boost::optional<std::string> _annotation_text;
	LocalTime _issue_date;
	boost::optional<int> _reel_number;
	Fraction _edit_rate;
	int _time_code_rate = 0;
	boost::optional<Time> _start_time;
	/** There are two SMPTE standards describing subtitles, 428-7:2010 and 428-7:2014, and they
	 *  have different interpretations of what Vposition means.  Though libdcp does not need to
	 *  know the difference, this variable stores the standard from the namespace that this asset was
	 *  written with (or will be written with).
	 */
	SubtitleStandard _subtitle_standard;

	std::vector<std::shared_ptr<SMPTELoadFontNode>> _load_font_nodes;
	/** UUID for the XML inside the MXF, which should be the same as the ResourceID in the MXF (our _resource_id)
	 *  but different to the AssetUUID in the MXF (our _id) according to SMPTE Bv2.1 and Doremi's 2.8.18 release notes.
	 *  May be boost::none if this object has been made from an encrypted object without a key.
	 */
	boost::optional<std::string> _xml_id;

	/** ResourceID read from the MXF, if there was one */
	boost::optional<std::string> _resource_id;
};


}


#endif
