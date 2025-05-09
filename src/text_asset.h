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


/** @file  src/text_asset.h
 *  @brief TextAsset class
 */


#ifndef LIBDCP_TEXT_ASSET_H
#define LIBDCP_TEXT_ASSET_H


#include "array_data.h"
#include "asset.h"
#include "dcp_time.h"
#include "load_variable_z.h"
#include "subtitle_standard.h"
#include "text_string.h"
#include <libcxml/cxml.h>
#include <boost/shared_array.hpp>
#include <map>
#include <string>
#include <utility>
#include <vector>


namespace xmlpp {
	class Document;
	class Element;
}


struct interop_dcp_font_test;
struct smpte_dcp_font_test;
struct pull_fonts_test1;
struct pull_fonts_test2;
struct pull_fonts_test3;


namespace dcp {


class FontNode;
class LoadFontNode;
class ReelAsset;
class SubtitleNode;
class TextImage;
class TextNode;
class TextString;


namespace order {
	class Part;
	struct Context;
}


/** @class TextAsset
 *  @brief A parent for classes representing a file containing subtitles or captions
 *
 *  This class holds a list of Text objects which it can extract
 *  from the appropriate part of either an Interop or SMPTE XML file.
 *  Its subclasses InteropTextAsset and SMPTETextAsset handle the
 *  differences between the two types.
 */
class TextAsset : public Asset
{
public:
	TextAsset();
	explicit TextAsset(boost::filesystem::path file);

	bool equals (
		std::shared_ptr<const Asset>,
		EqualityOptions const&,
		NoteHandler note
		) const override;

	std::vector<std::shared_ptr<const Text>> texts_during(Time from, Time to, bool starting) const;
	std::vector<std::shared_ptr<const Text>> texts() const;

	virtual void add(std::shared_ptr<Text>);
	virtual void add_font (std::string id, dcp::ArrayData data) = 0;
	void ensure_font(std::string id, dcp::ArrayData data);
	std::map<std::string, ArrayData> font_data () const;
	std::map<std::string, boost::filesystem::path> font_filenames () const;

	virtual void write (boost::filesystem::path) const = 0;
	virtual std::string xml_as_string () const = 0;

	Time latest_text_out() const;

	void fix_empty_font_ids ();

	virtual std::vector<std::shared_ptr<LoadFontNode>> load_font_nodes () const = 0;

	virtual int time_code_rate () const = 0;

	/** @return Raw XML loaded from, or written to, an on-disk asset, or boost::none if
	 *  - this object was not created from an existing on-disk asset and has not been written to one, or
	 *  - this asset is encrypted and no key is available.
	 */
	virtual boost::optional<std::string> raw_xml () const {
		return _raw_xml;
	}

	virtual SubtitleStandard subtitle_standard() const = 0;

	static std::string format_xml(xmlpp::Document const& document, boost::optional<std::pair<std::string, std::string>> xml_namespace);

protected:
	friend struct ::interop_dcp_font_test;
	friend struct ::smpte_dcp_font_test;

	struct ParseState {
		boost::optional<std::string> font_id;
		boost::optional<int64_t> size;
		boost::optional<float> aspect_adjust;
		boost::optional<bool> italic;
		boost::optional<bool> bold;
		boost::optional<bool> underline;
		boost::optional<Colour> colour;
		boost::optional<Effect> effect;
		boost::optional<Colour> effect_colour;
		boost::optional<float> h_position;
		boost::optional<HAlign> h_align;
		boost::optional<float> v_position;
		boost::optional<VAlign> v_align;
		boost::optional<float> z_position;
		boost::optional<std::string> variable_z;
		boost::optional<Direction> direction;
		boost::optional<Time> in;
		boost::optional<Time> out;
		boost::optional<Time> fade_up_time;
		boost::optional<Time> fade_down_time;
		enum class Type {
			TEXT,
			IMAGE
		};
		boost::optional<Type> type;
		float space_before = 0;

		std::vector<LoadVariableZ> load_variable_z;
	};

	void parse_texts(xmlpp::Element const * node, std::vector<ParseState>& state, boost::optional<int> tcr, Standard standard);
	ParseState font_node_state (xmlpp::Element const * node, Standard standard) const;
	ParseState text_node_state (xmlpp::Element const * node) const;
	ParseState image_node_state (xmlpp::Element const * node) const;
	ParseState subtitle_node_state (xmlpp::Element const * node, boost::optional<int> tcr) const;
	Time fade_time (xmlpp::Element const * node, std::string name, boost::optional<int> tcr) const;
	void position_align (ParseState& ps, xmlpp::Element const * node) const;

	void texts_as_xml(xmlpp::Element* root, int time_code_rate, Standard standard) const;

	/** All our texts, in no particular order */
	std::vector<std::shared_ptr<Text>> _texts;
	std::vector<LoadVariableZ> _load_variable_z;

	class Font
	{
	public:
		Font (std::string load_id_, std::string uuid_, boost::filesystem::path file_)
			: load_id (load_id_)
			, uuid (uuid_)
			, data (file_)
			, file (file_)
		{}

		Font (std::string load_id_, std::string uuid_, ArrayData data_)
			: load_id (load_id_)
			, uuid (uuid_)
			, data (data_)
		{}

		std::string load_id;
		std::string uuid;
		ArrayData data;
		/** .ttf file that this data was last written to, if applicable */
		mutable boost::optional<boost::filesystem::path> file;
	};

	/** TTF font data that we need */
	std::vector<Font> _fonts;

	/** The raw XML data that we read from or wrote to our asset; useful for validation */
	mutable boost::optional<std::string> _raw_xml;

private:
	friend struct ::pull_fonts_test1;
	friend struct ::pull_fonts_test2;
	friend struct ::pull_fonts_test3;

	void maybe_add_text(
		std::string text,
		std::vector<ParseState> const & parse_state,
		float space_before,
		Standard standard,
		std::vector<Ruby> const& rubies
		);

	static void pull_fonts (std::shared_ptr<order::Part> part);
};


}


#endif
