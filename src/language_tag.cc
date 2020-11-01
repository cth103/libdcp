/*
    Copyright (C) 2020 Carl Hetherington <cth@carlh.net>

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


#include "compose.hpp"
#include "dcp_assert.h"
#include "exceptions.h"
#include "language_tag.h"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <string>


using std::make_pair;
using std::ostream;
using std::pair;
using std::string;
using std::vector;
using boost::optional;
using boost::algorithm::trim;
using namespace dcp;




static vector<LanguageTag::SubtagData> language_list;
static vector<LanguageTag::SubtagData> variant_list;
static vector<LanguageTag::SubtagData> region_list;
static vector<LanguageTag::SubtagData> script_list;
static vector<LanguageTag::SubtagData> extlang_list;



static
optional<LanguageTag::SubtagData>
find_in_list (vector<LanguageTag::SubtagData> const& list, string subtag)
{
	BOOST_FOREACH (LanguageTag::SubtagData const& i, list) {
		if (i.subtag == subtag) {
			return i;
		}
	}

	return optional<LanguageTag::SubtagData>();
}


LanguageTag::Subtag::Subtag (string subtag, SubtagType type)
	: _subtag (subtag)
{
	if (!get_subtag_data(type, subtag)) {
		throw LanguageTagError(String::compose("Unknown %1 string %2", subtag_type_name(type), subtag));
	}
}


LanguageTag::LanguageTag (string tag)
{
	vector<string> parts;
	boost::split (parts, tag, boost::is_any_of("-"));
	if (parts.empty()) {
		throw LanguageTagError (String::compose("Could not parse language tag %1", tag));
	}

	vector<string>::size_type p = 0;
	_language = LanguageSubtag (parts[p]);
	++p;

	if (p == parts.size()) {
		return;
	}

	try {
		_script = ScriptSubtag (parts[p]);
		++p;
	} catch (...) {}

	if (p == parts.size()) {
		return;
	}

	try {
		_region = RegionSubtag (parts[p]);
		++p;
	} catch (...) {}

	if (p == parts.size()) {
		return;
	}

	try {
		while (true) {
			_variants.push_back (VariantSubtag(parts[p]));
			++p;
			if (p == parts.size()) {
				return;
			}
		}
	} catch (...) {}

	try {
		while (true) {
			_extlangs.push_back (ExtlangSubtag(parts[p]));
			++p;
			if (p == parts.size()) {
				return;
			}
		}
	} catch (...) {}

	if (p < parts.size()) {
		throw LanguageTagError (String::compose("Unrecognised subtag %1", parts[p]));
	}
}


string
LanguageTag::to_string () const
{
	if (!_language) {
		throw LanguageTagError("No language set up");
	}

	string s = _language->subtag();

	if (_script) {
		s += "-" + _script->subtag();
	}

	if (_region) {
		s += "-" + _region->subtag();
	}

	BOOST_FOREACH (VariantSubtag i, _variants) {
		s += "-" + i.subtag();
	}

	BOOST_FOREACH (ExtlangSubtag i, _extlangs) {
		s += "-" + i.subtag();
	}

	return s;
}


void
LanguageTag::set_language (LanguageSubtag language)
{
	_language = language;
}


void
LanguageTag::set_script (ScriptSubtag script)
{
	_script = script;
}


void
LanguageTag::set_region (RegionSubtag region)
{
	_region = region;
}


void
LanguageTag::add_variant (VariantSubtag variant)
{
	if (find(_variants.begin(), _variants.end(), variant) != _variants.end()) {
		throw LanguageTagError (String::compose("Duplicate Variant subtag %1", variant.subtag()));
	}

	_variants.push_back (variant);
}


template <class T>
void
check_for_duplicates (vector<T> const& subtags, dcp::LanguageTag::SubtagType type)
{
	vector<T> sorted = subtags;
	sort (sorted.begin(), sorted.end());
	optional<T> last;
	BOOST_FOREACH (T const& i, sorted) {
		if (last && i == *last) {
			throw LanguageTagError (String::compose("Duplicate %1 subtag %2", dcp::LanguageTag::subtag_type_name(type), i.subtag()));
		}
		last = i;
	}
}


void
LanguageTag::set_variants (vector<VariantSubtag> variants)
{
	check_for_duplicates (variants, VARIANT);
	_variants = variants;
}


void
LanguageTag::add_extlang (ExtlangSubtag extlang)
{
	if (find(_extlangs.begin(), _extlangs.end(), extlang) != _extlangs.end()) {
		throw LanguageTagError (String::compose("Duplicate Extlang subtag %1", extlang.subtag()));
	}

	_extlangs.push_back (extlang);
}


void
LanguageTag::set_extlangs (vector<ExtlangSubtag> extlangs)
{
	check_for_duplicates (extlangs, EXTLANG);
	_extlangs = extlangs;
}


string
LanguageTag::description () const
{
	if (!_language) {
		throw LanguageTagError("No language set up");
	}

	string d;

	BOOST_FOREACH (VariantSubtag const& i, _variants) {
		optional<SubtagData> variant = get_subtag_data (VARIANT, i.subtag());
		DCP_ASSERT (variant);
		d += variant->description + " dialect of ";
	}

	optional<SubtagData> language = get_subtag_data (LANGUAGE, _language->subtag());
	DCP_ASSERT (language);
	d += language->description;

	if (_script) {
		optional<SubtagData> script = get_subtag_data (SCRIPT, _script->subtag());
		DCP_ASSERT (script);
		d += " written using the " + script->description + " script";
	}

	if (_region) {
		optional<SubtagData> region = get_subtag_data (REGION, _region->subtag());
		DCP_ASSERT (region);
		d += " for " + region->description;
	}

	BOOST_FOREACH (ExtlangSubtag const& i, _extlangs) {
		optional<SubtagData> extlang = get_subtag_data (EXTLANG, i.subtag());
		DCP_ASSERT (extlang);
		d += ", " + extlang->description;
	}

	return d;
}


vector<LanguageTag::SubtagData> const &
LanguageTag::get_all (SubtagType type)
{
	switch (type) {
	case LANGUAGE:
		return language_list;
	case SCRIPT:
		return script_list;
	case REGION:
		return region_list;
	case VARIANT:
		return variant_list;
	case EXTLANG:
		return extlang_list;
	}

	return language_list;
}


string
LanguageTag::subtag_type_name (SubtagType type)
{
	switch (type) {
		case LANGUAGE:
			return "Language";
		case SCRIPT:
			return "Script";
		case REGION:
			return "Region";
		case VARIANT:
			return "Variant";
		case EXTLANG:
			return "Extended";
	}

	return "";
}

bool
dcp::LanguageTag::VariantSubtag::operator== (VariantSubtag const & other) const
{
	return subtag() == other.subtag();
}


bool
dcp::LanguageTag::VariantSubtag::operator< (VariantSubtag const & other) const
{
	return subtag() < other.subtag();
}


bool
dcp::LanguageTag::ExtlangSubtag::operator== (ExtlangSubtag const & other) const
{
	return subtag() == other.subtag();
}


bool
dcp::LanguageTag::ExtlangSubtag::operator< (ExtlangSubtag const & other) const
{
	return subtag() < other.subtag();
}


bool
dcp::operator== (dcp::LanguageTag const& a, dcp::LanguageTag const& b)
{
	return a.to_string() == b.to_string();
}


ostream&
dcp::operator<< (ostream& os, dcp::LanguageTag const& tag)
{
	os << tag.to_string();
	return os;
}


vector<pair<LanguageTag::SubtagType, LanguageTag::SubtagData> >
LanguageTag::subtags () const
{
	vector<pair<SubtagType, SubtagData> > s;

	if (_language) {
		s.push_back (make_pair(LANGUAGE, *get_subtag_data(LANGUAGE, _language->subtag())));
	}

	if (_script) {
		s.push_back (make_pair(SCRIPT, *get_subtag_data(SCRIPT, _script->subtag())));
	}

	if (_region) {
		s.push_back (make_pair(REGION, *get_subtag_data(REGION, _region->subtag())));
	}

	BOOST_FOREACH (VariantSubtag const& i, _variants) {
		s.push_back (make_pair(VARIANT, *get_subtag_data(VARIANT, i.subtag())));
	}

	BOOST_FOREACH (ExtlangSubtag const& i, _extlangs) {
		s.push_back (make_pair(EXTLANG, *get_subtag_data(EXTLANG, i.subtag())));
	}

	return s;
}


optional<LanguageTag::SubtagData>
LanguageTag::get_subtag_data (LanguageTag::SubtagType type, string subtag)
{
	switch (type) {
	case dcp::LanguageTag::LANGUAGE:
		return find_in_list(language_list, subtag);
	case dcp::LanguageTag::SCRIPT:
		return find_in_list(script_list, subtag);
	case dcp::LanguageTag::REGION:
		return find_in_list(region_list, subtag);
	case dcp::LanguageTag::VARIANT:
		return find_in_list(variant_list, subtag);
	case dcp::LanguageTag::EXTLANG:
		return find_in_list(extlang_list, subtag);
	}

	return optional<LanguageTag::SubtagData>();
}


optional<string>
LanguageTag::get_subtag_description (LanguageTag::SubtagType type, string subtag)
{
	optional<SubtagData> data = get_subtag_data (type, subtag);
	if (!data) {
		return optional<string>();
	}

	return data->description;
}


void
load_language_tag_list (boost::filesystem::path tags_directory, string name, vector<LanguageTag::SubtagData>& list)
{
	FILE* f = fopen_boost (tags_directory / name, "r");
	if (!f) {
		throw FileError ("Could not open tags file", tags_directory / name, errno);
	}
	char buffer[512];

	int i = 0;
	while (!feof(f)) {
		char* r = fgets (buffer, sizeof(buffer), f);
		if (r == 0) {
			break;
		}
		string a = buffer;
		trim (a);
		r = fgets (buffer, sizeof(buffer), f);
		if (r == 0) {
			fclose (f);
			throw FileError ("Bad tags file", tags_directory / name, -1);
		}
		string b = buffer;
		trim (b);
		list.push_back (LanguageTag::SubtagData(a, b));
		++i;
	}

	fclose (f);
}


void
dcp::load_language_tag_lists (boost::filesystem::path tags_directory)
{
	load_language_tag_list (tags_directory, "language", language_list);
	load_language_tag_list (tags_directory, "variant", variant_list);
	load_language_tag_list (tags_directory, "region", region_list);
	load_language_tag_list (tags_directory, "script", script_list);
	load_language_tag_list (tags_directory, "extlang", extlang_list);
}


