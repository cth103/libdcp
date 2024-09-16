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


/** @file  src/interop_subtitle_asset.cc
 *  @brief InteropSubtitleAsset class
 */


#include "compose.hpp"
#include "dcp_assert.h"
#include "equality_options.h"
#include "filesystem.h"
#include "font_asset.h"
#include "file.h"
#include "interop_load_font_node.h"
#include "interop_subtitle_asset.h"
#include "raw_convert.h"
#include "subtitle_asset_internal.h"
#include "subtitle_image.h"
#include "util.h"
#include "warnings.h"
#include "xml.h"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <boost/weak_ptr.hpp>
#include <cmath>
#include <cstdio>


using std::cerr;
using std::cout;
using std::dynamic_pointer_cast;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::vector;
using boost::optional;
using namespace dcp;


InteropSubtitleAsset::InteropSubtitleAsset (boost::filesystem::path file)
	: SubtitleAsset (file)
{
	_raw_xml = dcp::file_to_string(file, 10 * 1024 * 1024);

	auto xml = make_shared<cxml::Document>("DCSubtitle");
	xml->read_file(dcp::filesystem::fix_long_path(file));
	_id = xml->string_child ("SubtitleID");
	_reel_number = xml->string_child ("ReelNumber");
	_language = xml->string_child ("Language");
	_movie_title = xml->string_child ("MovieTitle");
	_load_font_nodes = type_children<InteropLoadFontNode> (xml, "LoadFont");

	/* Now we need to drop down to xmlpp */

	vector<ParseState> ps;
	for (auto i: xml->node()->get_children()) {
		auto e = dynamic_cast<xmlpp::Element const *>(i);
		if (e && (e->get_name() == "Font" || e->get_name() == "Subtitle")) {
			parse_subtitles (e, ps, optional<int>(), Standard::INTEROP);
		}
	}

	for (auto i: _subtitles) {
		auto si = dynamic_pointer_cast<SubtitleImage>(i);
		if (si) {
			si->read_png_file (file.parent_path() / String::compose("%1.png", si->id()));
		}
	}
}


InteropSubtitleAsset::InteropSubtitleAsset ()
{

}


string
InteropSubtitleAsset::xml_as_string () const
{
	xmlpp::Document doc;
	auto root = doc.create_root_node ("DCSubtitle");
	root->set_attribute ("Version", "1.0");

	root->add_child("SubtitleID")->add_child_text (_id);
	root->add_child("MovieTitle")->add_child_text (_movie_title);
	root->add_child("ReelNumber")->add_child_text (raw_convert<string> (_reel_number));
	root->add_child("Language")->add_child_text (_language);

	for (auto i: _load_font_nodes) {
		auto load_font = root->add_child("LoadFont");
		load_font->set_attribute ("Id", i->id);
		load_font->set_attribute ("URI", i->uri);
	}

	subtitles_as_xml (root, 250, Standard::INTEROP);

	return format_xml(doc, {});
}


void
InteropSubtitleAsset::add_font (string load_id, dcp::ArrayData data)
{
	_fonts.push_back (Font(load_id, make_uuid(), data));
	auto const uri = String::compose("font_%1.ttf", _load_font_nodes.size());
	_load_font_nodes.push_back (make_shared<InteropLoadFontNode>(load_id, uri));
}


bool
InteropSubtitleAsset::equals(shared_ptr<const Asset> other_asset, EqualityOptions const& options, NoteHandler note) const
{
	if (!SubtitleAsset::equals (other_asset, options, note)) {
		return false;
	}

	auto other = dynamic_pointer_cast<const InteropSubtitleAsset> (other_asset);
	if (!other) {
		return false;
	}

	if (!options.load_font_nodes_can_differ) {
		auto i = _load_font_nodes.begin();
		auto j = other->_load_font_nodes.begin();

		while (i != _load_font_nodes.end ()) {
			if (j == other->_load_font_nodes.end ()) {
				note (NoteType::ERROR, "<LoadFont> nodes differ");
				return false;
			}

			if (**i != **j) {
				note (NoteType::ERROR, "<LoadFont> nodes differ");
				return false;
			}

			++i;
			++j;
		}
	}

	if (_movie_title != other->_movie_title) {
		note (NoteType::ERROR, "Subtitle movie titles differ");
		return false;
	}

	return true;
}


vector<shared_ptr<LoadFontNode>>
InteropSubtitleAsset::load_font_nodes () const
{
	vector<shared_ptr<LoadFontNode>> lf;
	copy (_load_font_nodes.begin(), _load_font_nodes.end(), back_inserter (lf));
	return lf;
}


void
InteropSubtitleAsset::write (boost::filesystem::path p) const
{
	File f(p, "wb");
	if (!f) {
		throw FileError ("Could not open file for writing", p, -1);
	}

	_raw_xml = xml_as_string ();
	/* length() here gives bytes not characters */
	f.write(_raw_xml->c_str(), 1, _raw_xml->length());

	_file = p;

	/* Image subtitles */
	for (auto i: _subtitles) {
		auto im = dynamic_pointer_cast<dcp::SubtitleImage> (i);
		if (im) {
			im->write_png_file(p.parent_path() / String::compose("%1.png", im->id()));
		}
	}

	/* Fonts */
	for (auto i: _load_font_nodes) {
		auto file = p.parent_path() / i->uri;
		auto font_with_id = std::find_if(_fonts.begin(), _fonts.end(), [i](Font const& font) { return font.load_id == i->id; });
		if (font_with_id != _fonts.end()) {
			font_with_id->data.write(file);
			font_with_id->file = file;
		}
	}
}


/** Look at a supplied list of assets and find the fonts.  Then match these
 *  fonts up with anything requested by a <LoadFont> so that _fonts contains
 *  a list of font ID, load ID and data.
 */
void
InteropSubtitleAsset::resolve_fonts (vector<shared_ptr<Asset>> assets)
{
	for (auto asset: assets) {
		auto font = dynamic_pointer_cast<FontAsset>(asset);
		if (!font) {
			continue;
		}

		DCP_ASSERT(_file);

		for (auto load_font_node: _load_font_nodes) {
			auto const path_in_load_font_node = _file->parent_path() / load_font_node->uri;
			if (font->file() && path_in_load_font_node == *font->file()) {
				auto existing = std::find_if(_fonts.begin(), _fonts.end(), [load_font_node](Font const& font) { return font.load_id == load_font_node->id; });
				if (existing != _fonts.end()) {
					*existing = Font(load_font_node->id, asset->id(), font->file().get());
				} else {
					_fonts.push_back(Font(load_font_node->id, asset->id(), font->file().get()));
				}
			}
		}
	}
}


vector<shared_ptr<Asset>>
InteropSubtitleAsset::font_assets()
{
	vector<shared_ptr<Asset>> assets;
	for (auto const& i: _fonts) {
		DCP_ASSERT (i.file);
		assets.push_back(make_shared<FontAsset>(i.uuid, i.file.get()));
	}
	return assets;
}


vector<shared_ptr<const Asset>>
InteropSubtitleAsset::font_assets() const
{
	vector<shared_ptr<const Asset>> assets;
	for (auto const& i: _fonts) {
		DCP_ASSERT (i.file);
		assets.push_back(make_shared<const FontAsset>(i.uuid, i.file.get()));
	}
	return assets;
}


void
InteropSubtitleAsset::add_to_assetmap (AssetMap& asset_map, boost::filesystem::path root) const
{
	Asset::add_to_assetmap(asset_map, root);

	for (auto i: _subtitles) {
		auto im = dynamic_pointer_cast<dcp::SubtitleImage>(i);
		if (im) {
			DCP_ASSERT(im->file());
			add_file_to_assetmap(asset_map, root, im->file().get(), im->id());
		}
	}
}


void
InteropSubtitleAsset::add_to_pkl (shared_ptr<PKL> pkl, boost::filesystem::path root) const
{
	Asset::add_to_pkl (pkl, root);

	for (auto i: _subtitles) {
		auto im = dynamic_pointer_cast<dcp::SubtitleImage> (i);
		if (im) {
			auto png_image = im->png_image ();
			pkl->add_asset(im->id(), optional<string>(), make_digest(png_image), png_image.size(), "image/png", root.filename().string());
		}
	}
}


void
InteropSubtitleAsset::set_font_file (string load_id, boost::filesystem::path file)
{
	for (auto& i: _fonts) {
		if (i.load_id == load_id) {
			i.file = file;
		}
	}

	for (auto i: _load_font_nodes) {
		if (i->id == load_id) {
			i->uri = file.filename().string();
		}
	}
}


vector<string>
InteropSubtitleAsset::unresolved_fonts() const
{
	vector<string> unresolved;
	for (auto load_font_node: _load_font_nodes) {
		if (std::find_if(_fonts.begin(), _fonts.end(), [load_font_node](Font const& font) { return font.load_id == load_font_node->id; }) == _fonts.end()) {
			unresolved.push_back(load_font_node->id);
		}
	}
	return unresolved;
}

