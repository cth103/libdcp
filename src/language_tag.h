/*
    Copyright (C) 2020-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/language_tag.cc
 *  @brief LanguageTag class
 */


#ifndef LIBDCP_LANGUAGE_TAG_H
#define LIBDCP_LANGUAGE_TAG_H


#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <string>
#include <vector>


namespace dcp {


class LanguageTag
{
public:
	std::string to_string () const;
	std::string description () const;

	struct SubtagData
	{
		SubtagData () {}

		SubtagData (std::string subtag_, std::string description_)
			: subtag(subtag_)
			, description(description_)
		{}

		std::string subtag;
		std::string description;

		bool operator== (SubtagData const& other) {
			return subtag == other.subtag && description == other.description;
		}
	};

	enum class SubtagType
	{
		LANGUAGE,
		SCRIPT,
		REGION,
		VARIANT,
		EXTLANG,
	};

	class Subtag
	{
	public:
		virtual ~Subtag () {}

		std::string subtag () const {
			return _subtag;
		}

		virtual SubtagType type () const = 0;

		bool operator== (Subtag const& other) {
			return _subtag == other._subtag;
		}

	protected:
		Subtag (std::string subtag, SubtagType type);

	private:
		std::string _subtag;
	};

	class LanguageSubtag : public Subtag
	{
	public:
		LanguageSubtag (std::string subtag)
			: Subtag(subtag, SubtagType::LANGUAGE) {}
		LanguageSubtag (char const* subtag)
			: Subtag(subtag, SubtagType::LANGUAGE) {}

		SubtagType type () const override {
			return SubtagType::LANGUAGE;
		}
	};

	class ScriptSubtag : public Subtag
	{
	public:
		ScriptSubtag (std::string subtag)
			: Subtag(subtag, SubtagType::SCRIPT) {}
		ScriptSubtag (char const* subtag)
			: Subtag(subtag, SubtagType::SCRIPT) {}

		SubtagType type () const override {
			return SubtagType::SCRIPT;
		}
	};

	class RegionSubtag : public Subtag
	{
	public:
		RegionSubtag (std::string subtag)
			: Subtag(subtag, SubtagType::REGION) {}
		RegionSubtag (char const* subtag)
			: Subtag(subtag, SubtagType::REGION) {}

		SubtagType type () const override {
			return SubtagType::REGION;
		}
	};

	class VariantSubtag : public Subtag
	{
	public:
		VariantSubtag (std::string subtag)
			: Subtag(subtag, SubtagType::VARIANT) {}
		VariantSubtag (char const* subtag)
			: Subtag(subtag, SubtagType::VARIANT) {}

		SubtagType type () const override {
			return SubtagType::VARIANT;
		}

		bool operator== (VariantSubtag const& other) const;
		bool operator< (VariantSubtag const& other) const;
	};


	class ExtlangSubtag : public Subtag
	{
	public:
		ExtlangSubtag (std::string subtag)
			: Subtag(subtag, SubtagType::EXTLANG) {}
		ExtlangSubtag (char const* subtag)
			: Subtag(subtag, SubtagType::EXTLANG) {}

		SubtagType type () const override {
			return SubtagType::EXTLANG;
		}

		bool operator== (ExtlangSubtag const& other) const;
		bool operator< (ExtlangSubtag const& other) const;
	};

	LanguageTag () {}
	explicit LanguageTag (std::string tag);

	boost::optional<LanguageSubtag> language() const {
		return _language;
	}

	void set_language (LanguageSubtag language);

	boost::optional<ScriptSubtag> script() const {
		return _script;
	}

	void set_script (ScriptSubtag script);

	boost::optional<RegionSubtag> region() const {
		return _region;
	}

	void set_region (RegionSubtag region);

	std::vector<VariantSubtag> variants() const {
		return _variants;
	}

	void set_variants (std::vector<VariantSubtag> variants);
	void add_variant (VariantSubtag variant);

	std::vector<ExtlangSubtag> extlangs() const {
		return _extlangs;
	}

	void set_extlangs (std::vector<ExtlangSubtag> extlangs);
	void add_extlang (ExtlangSubtag extlang);

	std::vector<std::pair<SubtagType, SubtagData>> subtags () const;

	static std::vector<SubtagData> const& get_all (SubtagType type);
	static std::string subtag_type_name (SubtagType type);

	static boost::optional<std::string> get_subtag_description (SubtagType, std::string subtag);
	static boost::optional<SubtagData> get_subtag_data (SubtagType, std::string subtag);

	template <class T>
	static boost::optional<std::string> get_subtag_description (T s) {
		return get_subtag_description (s.type(), s.subtag());
	}

	template <class T>
	static boost::optional<SubtagData> get_subtag_data (T s) {
		return get_subtag_data (s.type(), s.subtag());
	}

private:

	boost::optional<LanguageSubtag> _language;
	boost::optional<ScriptSubtag> _script;
	boost::optional<RegionSubtag> _region;
	std::vector<VariantSubtag> _variants;
	std::vector<ExtlangSubtag> _extlangs;
};


extern bool operator==(dcp::LanguageTag const& a, dcp::LanguageTag const& b);
extern bool operator!=(dcp::LanguageTag const& a, dcp::LanguageTag const& b);
extern bool operator<(dcp::LanguageTag const& a, dcp::LanguageTag const& b);
extern std::ostream& operator<<(std::ostream& os, dcp::LanguageTag const& tag);


extern void load_language_tag_lists (boost::filesystem::path tags_directory);


extern std::vector<std::pair<std::string, std::string>> dcnc_tags ();


}


#endif
