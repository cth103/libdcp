/*
    Copyright (C) 2012-2018 Carl Hetherington <cth@carlh.net>

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

#include "interop_subtitle_asset.h"
#include "interop_load_font_node.h"
#include "subtitle_asset_internal.h"
#include "xml.h"
#include "raw_convert.h"
#include "util.h"
#include "font_asset.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include "subtitle_image.h"
#include <libxml++/libxml++.h>
#include <boost/foreach.hpp>
#include <boost/weak_ptr.hpp>
#include <cmath>
#include <cstdio>

using std::list;
using std::string;
using std::cout;
using std::cerr;
using std::map;
using boost::shared_ptr;
using boost::shared_array;
using boost::optional;
using boost::dynamic_pointer_cast;
using namespace dcp;

InteropSubtitleAsset::InteropSubtitleAsset (boost::filesystem::path file)
	: SubtitleAsset (file)
{
	_raw_xml = dcp::file_to_string (file);

	shared_ptr<cxml::Document> xml (new cxml::Document ("DCSubtitle"));
	xml->read_file (file);
	_id = xml->string_child ("SubtitleID");
	_reel_number = xml->string_child ("ReelNumber");
	_language = xml->string_child ("Language");
	_movie_title = xml->string_child ("MovieTitle");
	_load_font_nodes = type_children<InteropLoadFontNode> (xml, "LoadFont");

	/* Now we need to drop down to xmlpp */

	list<ParseState> ps;
	xmlpp::Node::NodeList c = xml->node()->get_children ();
	for (xmlpp::Node::NodeList::const_iterator i = c.begin(); i != c.end(); ++i) {
		xmlpp::Element const * e = dynamic_cast<xmlpp::Element const *> (*i);
		if (e && (e->get_name() == "Font" || e->get_name() == "Subtitle")) {
			parse_subtitles (e, ps, optional<int>(), INTEROP);
		}
	}

	BOOST_FOREACH (shared_ptr<Subtitle> i, _subtitles) {
		shared_ptr<SubtitleImage> si = dynamic_pointer_cast<SubtitleImage>(i);
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
	xmlpp::Element* root = doc.create_root_node ("DCSubtitle");
	root->set_attribute ("Version", "1.0");

	root->add_child("SubtitleID")->add_child_text (_id);
	root->add_child("MovieTitle")->add_child_text (_movie_title);
	root->add_child("ReelNumber")->add_child_text (raw_convert<string> (_reel_number));
	root->add_child("Language")->add_child_text (_language);

	for (list<shared_ptr<InteropLoadFontNode> >::const_iterator i = _load_font_nodes.begin(); i != _load_font_nodes.end(); ++i) {
		xmlpp::Element* load_font = root->add_child("LoadFont");
		load_font->set_attribute ("Id", (*i)->id);
		load_font->set_attribute ("URI", (*i)->uri);
	}

	subtitles_as_xml (root, 250, INTEROP);

	return doc.write_to_string ("UTF-8");
}

void
InteropSubtitleAsset::add_font (string load_id, dcp::ArrayData data)
{
	_fonts.push_back (Font(load_id, make_uuid(), data));
	string const uri = String::compose("font_%1.ttf", _load_font_nodes.size());
	_load_font_nodes.push_back (shared_ptr<InteropLoadFontNode>(new InteropLoadFontNode(load_id, uri)));
}

bool
InteropSubtitleAsset::equals (shared_ptr<const Asset> other_asset, EqualityOptions options, NoteHandler note) const
{
	if (!SubtitleAsset::equals (other_asset, options, note)) {
		return false;
	}

	shared_ptr<const InteropSubtitleAsset> other = dynamic_pointer_cast<const InteropSubtitleAsset> (other_asset);
	if (!other) {
		return false;
	}

	if (!options.load_font_nodes_can_differ) {
		list<shared_ptr<InteropLoadFontNode> >::const_iterator i = _load_font_nodes.begin ();
		list<shared_ptr<InteropLoadFontNode> >::const_iterator j = other->_load_font_nodes.begin ();

		while (i != _load_font_nodes.end ()) {
			if (j == other->_load_font_nodes.end ()) {
				note (DCP_ERROR, "<LoadFont> nodes differ");
				return false;
			}

			if (**i != **j) {
				note (DCP_ERROR, "<LoadFont> nodes differ");
				return false;
			}

			++i;
			++j;
		}
	}

	if (_movie_title != other->_movie_title) {
		note (DCP_ERROR, "Subtitle movie titles differ");
		return false;
	}

	return true;
}

list<shared_ptr<LoadFontNode> >
InteropSubtitleAsset::load_font_nodes () const
{
	list<shared_ptr<LoadFontNode> > lf;
	copy (_load_font_nodes.begin(), _load_font_nodes.end(), back_inserter (lf));
	return lf;
}

/** Write this content to an XML file with its fonts alongside */
void
InteropSubtitleAsset::write (boost::filesystem::path p) const
{
	FILE* f = fopen_boost (p, "w");
	if (!f) {
		throw FileError ("Could not open file for writing", p, -1);
	}

	string const s = xml_as_string ();
	/* length() here gives bytes not characters */
	fwrite (s.c_str(), 1, s.length(), f);
	fclose (f);

	_file = p;

	/* Image subtitles */
	BOOST_FOREACH (shared_ptr<dcp::Subtitle> i, _subtitles) {
		shared_ptr<dcp::SubtitleImage> im = dynamic_pointer_cast<dcp::SubtitleImage> (i);
		if (im) {
			im->write_png_file(p.parent_path() / String::compose("%1.png", im->id()));
		}
	}

	/* Fonts */
	BOOST_FOREACH (shared_ptr<InteropLoadFontNode> i, _load_font_nodes) {
		boost::filesystem::path file = p.parent_path() / i->uri;
		list<Font>::const_iterator j = _fonts.begin ();
		while (j != _fonts.end() && j->load_id != i->id) {
			++j;
		}
		if (j != _fonts.end ()) {
			j->data.write (file);
			j->file = file;
		}
	}
}

/** Look at a supplied list of assets and find the fonts.  Then match these
 *  fonts up with anything requested by a <LoadFont> so that _fonts contains
 *  a list of font ID, load ID and data.
 */
void
InteropSubtitleAsset::resolve_fonts (list<shared_ptr<Asset> > assets)
{
	BOOST_FOREACH (shared_ptr<Asset> i, assets) {
		shared_ptr<FontAsset> font = dynamic_pointer_cast<FontAsset> (i);
		if (!font) {
			continue;
		}

		BOOST_FOREACH (shared_ptr<InteropLoadFontNode> j, _load_font_nodes) {
			bool got = false;
			BOOST_FOREACH (Font const & k, _fonts) {
				if (k.load_id == j->id) {
					got = true;
					break;
				}
			}

			if (!got && font->file() && j->uri == font->file()->leaf().string()) {
				_fonts.push_back (Font (j->id, i->id(), font->file().get()));
			}
		}
	}
}

void
InteropSubtitleAsset::add_font_assets (list<shared_ptr<Asset> >& assets)
{
	BOOST_FOREACH (Font const & i, _fonts) {
		DCP_ASSERT (i.file);
		assets.push_back (shared_ptr<FontAsset> (new FontAsset (i.uuid, i.file.get ())));
	}
}

void
InteropSubtitleAsset::write_to_assetmap (xmlpp::Node* node, boost::filesystem::path root) const
{
	Asset::write_to_assetmap (node, root);

	BOOST_FOREACH (shared_ptr<dcp::Subtitle> i, _subtitles) {
		shared_ptr<dcp::SubtitleImage> im = dynamic_pointer_cast<dcp::SubtitleImage> (i);
		if (im) {
			DCP_ASSERT (im->file());
			write_file_to_assetmap (node, root, im->file().get(), im->id());
		}
	}
}

void
InteropSubtitleAsset::add_to_pkl (shared_ptr<PKL> pkl, boost::filesystem::path root) const
{
	Asset::add_to_pkl (pkl, root);

	BOOST_FOREACH (shared_ptr<dcp::Subtitle> i, _subtitles) {
		shared_ptr<dcp::SubtitleImage> im = dynamic_pointer_cast<dcp::SubtitleImage> (i);
		if (im) {
			ArrayData png_image = im->png_image ();
			pkl->add_asset (im->id(), optional<string>(), make_digest(png_image), png_image.size(), "image/png");
		}
	}
}


void
InteropSubtitleAsset::set_font_file (string load_id, boost::filesystem::path file)
{
	BOOST_FOREACH (Font& i, _fonts) {
		if (i.load_id == load_id) {
			i.file = file;
		}
	}

	BOOST_FOREACH (shared_ptr<InteropLoadFontNode> i, _load_font_nodes) {
		if (i->id == load_id) {
			i->uri = file.filename().string();
		}
	}
}

