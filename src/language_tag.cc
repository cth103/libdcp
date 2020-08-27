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
#include <boost/foreach.hpp>
#include <string>


using std::string;
using std::vector;
using boost::optional;
using namespace dcp;


#include "language_tag_lists.cc"


static
optional<LanguageTag::SubtagData>
find_in_list (LanguageTag::SubtagData* list, int length, string subtag)
{
	for (int i = 0; i < length; ++i) {
		if (list[i].subtag == subtag) {
			return list[i];
		}
	}

	return optional<LanguageTag::SubtagData>();
}


static
optional<LanguageTag::SubtagData>
find_in_list (LanguageTag::SubtagType type, string subtag)
{
	switch (type) {
	case dcp::LanguageTag::LANGUAGE:
		return find_in_list(language_list, sizeof(language_list) / sizeof(LanguageTag::SubtagData), subtag);
	case dcp::LanguageTag::SCRIPT:
		return find_in_list(script_list, sizeof(script_list) / sizeof(LanguageTag::SubtagData), subtag);
	case dcp::LanguageTag::REGION:
		return find_in_list(region_list, sizeof(region_list) / sizeof(LanguageTag::SubtagData), subtag);
	case dcp::LanguageTag::VARIANT:
		return find_in_list(variant_list, sizeof(variant_list) / sizeof(LanguageTag::SubtagData), subtag);
	case dcp::LanguageTag::EXTLANG:
		return find_in_list(extlang_list, sizeof(extlang_list) / sizeof(LanguageTag::SubtagData), subtag);
	}

	return optional<LanguageTag::SubtagData>();
}


LanguageTag::Subtag::Subtag (string subtag, SubtagType type)
	: _subtag (subtag)
{
	if (!find_in_list(type, subtag)) {
		throw LanguageTagError(String::compose("Unknown %1 string %2", subtag_type_name(type), subtag));
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
		optional<SubtagData> variant = find_in_list (VARIANT, i.subtag());
		DCP_ASSERT (variant);
		d += variant->description + " dialect of ";
	}

	optional<SubtagData> language = find_in_list (LANGUAGE, _language->subtag());
	DCP_ASSERT (language);
	d += language->description;

	if (_script) {
		optional<SubtagData> script = find_in_list (SCRIPT, _script->subtag());
		DCP_ASSERT (script);
		d += " written using the " + script->description + " script";
	}

	if (_region) {
		optional<SubtagData> region = find_in_list (REGION, _region->subtag());
		DCP_ASSERT (region);
		d += " for " + region->description;
	}

	BOOST_FOREACH (ExtlangSubtag const& i, _extlangs) {
		optional<SubtagData> extlang = find_in_list (EXTLANG, i.subtag());
		DCP_ASSERT (extlang);
		d += ", " + extlang->description;
	}

	return d;
}


vector<LanguageTag::SubtagData>
LanguageTag::get_all (SubtagType type)
{
	vector<LanguageTag::SubtagData> all;

	switch (type) {
	case LANGUAGE:
		for (size_t i = 0; i < sizeof(language_list) / sizeof(LanguageTag::SubtagData); ++i) {
			all.push_back (language_list[i]);
		}
		break;
	case SCRIPT:
		for (size_t i = 0; i < sizeof(script_list) / sizeof(LanguageTag::SubtagData); ++i) {
			all.push_back (script_list[i]);
		}
		break;
	case REGION:
		for (size_t i = 0; i < sizeof(region_list) / sizeof(LanguageTag::SubtagData); ++i) {
			all.push_back (region_list[i]);
		}
		break;
	case VARIANT:
		for (size_t i = 0; i < sizeof(variant_list) / sizeof(LanguageTag::SubtagData); ++i) {
			all.push_back (variant_list[i]);
		}
		break;
	case EXTLANG:
		for (size_t i = 0; i < sizeof(extlang_list) / sizeof(LanguageTag::SubtagData); ++i) {
			all.push_back (extlang_list[i]);
		}
		break;
	}

	return all;
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