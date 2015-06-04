/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "smpte_subtitle_asset.h"
#include "smpte_load_font_node.h"
#include "font_node.h"
#include "exceptions.h"
#include "xml.h"
#include "AS_DCP.h"
#include "KM_util.h"
#include "raw_convert.h"
#include <libxml++/libxml++.h>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

using std::string;
using std::list;
using std::stringstream;
using std::cout;
using std::vector;
using boost::shared_ptr;
using boost::split;
using boost::is_any_of;
using namespace dcp;

SMPTESubtitleAsset::SMPTESubtitleAsset (boost::filesystem::path file, bool mxf)
	: SubtitleAsset (file)
{
	shared_ptr<cxml::Document> xml (new cxml::Document ("SubtitleReel"));
	
	if (mxf) {
		ASDCP::TimedText::MXFReader reader;
		Kumu::Result_t r = reader.OpenRead (file.string().c_str ());
		if (ASDCP_FAILURE (r)) {
			boost::throw_exception (MXFFileError ("could not open MXF file for reading", file, r));
		}
	
		string s;
		reader.ReadTimedTextResource (s, 0, 0);
		stringstream t;
		t << s;
		xml->read_stream (t);

		ASDCP::WriterInfo info;
		reader.FillWriterInfo (info);
		
		char buffer[64];
		Kumu::bin2UUIDhex (info.AssetUUID, ASDCP::UUIDlen, buffer, sizeof (buffer));
		_id = buffer;
	} else {
		xml->read_file (file);
		_id = xml->string_child("Id").substr (9);
	}
	
	_load_font_nodes = type_children<dcp::SMPTELoadFontNode> (xml, "LoadFont");

	_content_title_text = xml->string_child ("ContentTitleText");
	_annotation_text = xml->optional_string_child ("AnnotationText");
	_issue_date = LocalTime (xml->string_child ("IssueDate"));
	_reel_number = xml->optional_number_child<int> ("ReelNumber");
	_language = xml->optional_string_child ("Language");

	/* This is supposed to be two numbers, but a single number has been seen in the wild */
	string const er = xml->string_child ("EditRate");
	vector<string> er_parts;
	split (er_parts, er, is_any_of (" "));
	if (er_parts.size() == 1) {
		_edit_rate = Fraction (raw_convert<int> (er_parts[0]), 1);
	} else if (er_parts.size() == 2) {
		_edit_rate = Fraction (raw_convert<int> (er_parts[0]), raw_convert<int> (er_parts[1]));
	} else {
		throw XMLError ("malformed EditRate " + er);
	}

	_time_code_rate = xml->number_child<int> ("TimeCodeRate");
	if (xml->optional_string_child ("StartTime")) {
		_start_time = Time (xml->string_child ("StartTime"), _time_code_rate);
	}

	shared_ptr<cxml::Node> subtitle_list = xml->optional_node_child ("SubtitleList");

	list<cxml::NodePtr> f = subtitle_list->node_children ("Font");
	list<shared_ptr<dcp::FontNode> > font_nodes;
	BOOST_FOREACH (cxml::NodePtr& i, f) {
		font_nodes.push_back (shared_ptr<FontNode> (new FontNode (i, _time_code_rate)));
	}
	
	parse_common (xml, font_nodes);
}

list<shared_ptr<LoadFontNode> >
SMPTESubtitleAsset::load_font_nodes () const
{
	list<shared_ptr<LoadFontNode> > lf;
	copy (_load_font_nodes.begin(), _load_font_nodes.end(), back_inserter (lf));
	return lf;
}

bool
SMPTESubtitleAsset::valid_mxf (boost::filesystem::path file)
{
	ASDCP::TimedText::MXFReader reader;
	Kumu::Result_t r = reader.OpenRead (file.string().c_str ());
	return !ASDCP_FAILURE (r);
}

Glib::ustring
SMPTESubtitleAsset::xml_as_string () const
{
	xmlpp::Document doc;
	xmlpp::Element* root = doc.create_root_node ("dcst:SubtitleReel");
	root->set_namespace_declaration ("http://www.smpte-ra.org/schemas/428-7/2010/DCST", "dcst");
	root->set_namespace_declaration ("http://www.w3.org/2001/XMLSchema", "xs");

	root->add_child("ID", "dcst")->add_child_text (_id);
	root->add_child("ContentTitleText", "dcst")->add_child_text (_content_title_text);
	if (_annotation_text) {
		root->add_child("AnnotationText", "dcst")->add_child_text (_annotation_text.get ());
	}
	root->add_child("IssueDate", "dcst")->add_child_text (_issue_date.as_string (true));
	if (_reel_number) {
		root->add_child("ReelNumber", "dcst")->add_child_text (raw_convert<string> (_reel_number.get ()));
	}
	if (_language) {
		root->add_child("Language", "dcst")->add_child_text (_language.get ());
	}
	root->add_child("EditRate", "dcst")->add_child_text (_edit_rate.as_string ());
	root->add_child("TimeCodeRate", "dcst")->add_child_text (raw_convert<string> (_time_code_rate));
	if (_start_time) {
		root->add_child("StartTime", "dcst")->add_child_text (_start_time.get().as_string ());
	}

	BOOST_FOREACH (shared_ptr<SMPTELoadFontNode> i, _load_font_nodes) {
		xmlpp::Element* load_font = root->add_child("LoadFont", "dcst");
		load_font->add_child_text (i->urn);
		load_font->set_attribute ("ID", i->id);
	}
	
	subtitles_as_xml (root->add_child ("SubtitleList", "dcst"), _time_code_rate, "dcst");
	
	return doc.write_to_string_formatted ("UTF-8");
}

/** Write this content to a MXF file */
void
SMPTESubtitleAsset::write (boost::filesystem::path p) const
{
	ASDCP::WriterInfo writer_info;
	fill_writer_info (&writer_info, _id, SMPTE);
	
	ASDCP::TimedText::TimedTextDescriptor descriptor;
	descriptor.EditRate = ASDCP::Rational (_edit_rate.numerator, _edit_rate.denominator);
	descriptor.EncodingName = "UTF-8";
	descriptor.ResourceList.clear ();
	descriptor.NamespaceName = "dcst";
	memcpy (descriptor.AssetID, writer_info.AssetUUID, ASDCP::UUIDlen);
	descriptor.ContainerDuration = latest_subtitle_out().as_editable_units (_edit_rate.numerator / _edit_rate.denominator);

	/* XXX: fonts into descriptor? */
	
	ASDCP::TimedText::MXFWriter writer;
	Kumu::Result_t r = writer.OpenWrite (p.string().c_str(), writer_info, descriptor);
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (FileError ("could not open subtitle MXF for writing", p.string(), r));
	}

	/* XXX: no encryption */
	writer.WriteTimedTextResource (xml_as_string ());

	writer.Finalize ();

	_file = p;
}

bool
SMPTESubtitleAsset::equals (shared_ptr<const Asset> other_asset, EqualityOptions options, NoteHandler note) const
{
	/* XXX */
	return false;
}

