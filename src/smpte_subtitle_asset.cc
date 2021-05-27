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


/** @file  src/smpte_subtitle_asset.cc
 *  @brief SMPTESubtitleAsset class
 */


#include "compose.hpp"
#include "crypto_context.h"
#include "dcp_assert.h"
#include "exceptions.h"
#include "raw_convert.h"
#include "smpte_load_font_node.h"
#include "smpte_subtitle_asset.h"
#include "subtitle_image.h"
#include "util.h"
#include "warnings.h"
#include "xml.h"
LIBDCP_DISABLE_WARNINGS
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_util.h>
#include <asdcp/KM_log.h>
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <boost/algorithm/string.hpp>


using std::string;
using std::list;
using std::vector;
using std::map;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using std::make_shared;
using boost::split;
using boost::is_any_of;
using boost::shared_array;
using boost::optional;
using boost::starts_with;
using namespace dcp;


static string const subtitle_smpte_ns = "http://www.smpte-ra.org/schemas/428-7/2010/DCST";


SMPTESubtitleAsset::SMPTESubtitleAsset ()
	: MXF (Standard::SMPTE)
	, _edit_rate (24, 1)
	, _time_code_rate (24)
	, _xml_id (make_uuid())
{

}


SMPTESubtitleAsset::SMPTESubtitleAsset (boost::filesystem::path file)
	: SubtitleAsset (file)
{
	auto xml = make_shared<cxml::Document>("SubtitleReel");

	auto reader = make_shared<ASDCP::TimedText::MXFReader>();
	auto r = Kumu::RESULT_OK;
	{
		ASDCPErrorSuspender sus;
		r = reader->OpenRead (_file->string().c_str ());
	}
	if (!ASDCP_FAILURE(r)) {
		/* MXF-wrapped */
		ASDCP::WriterInfo info;
		reader->FillWriterInfo (info);
		_id = read_writer_info (info);
		if (!_key_id) {
			/* Not encrypted; read it in now */
			string xml_string;
			reader->ReadTimedTextResource (xml_string);
			_raw_xml = xml_string;
			xml->read_string (xml_string);
			parse_xml (xml);
			read_mxf_descriptor (reader);
			read_mxf_resources (reader, make_shared<DecryptionContext>(optional<Key>(), Standard::SMPTE));
		} else {
			read_mxf_descriptor (reader);
		}
	} else {
		/* Plain XML */
		try {
			_raw_xml = dcp::file_to_string (file);
			xml = make_shared<cxml::Document>("SubtitleReel");
			xml->read_file (file);
			parse_xml (xml);
		} catch (cxml::Error& e) {
			boost::throw_exception (
				ReadError (
					String::compose (
						"Failed to read subtitle file %1; MXF failed with %2, XML failed with %3",
						file, static_cast<int>(r), e.what()
						)
					)
				);
		}

		/* Try to read PNG files from the same folder that the XML is in; the wisdom of this is
		   debatable, at best...
		*/
		for (auto i: _subtitles) {
			auto im = dynamic_pointer_cast<SubtitleImage>(i);
			if (im && im->png_image().size() == 0) {
				/* Even more dubious; allow <id>.png or urn:uuid:<id>.png */
				auto p = file.parent_path() / String::compose("%1.png", im->id());
				if (boost::filesystem::is_regular_file(p)) {
					im->read_png_file (p);
				} else if (starts_with (im->id(), "urn:uuid:")) {
					p = file.parent_path() / String::compose("%1.png", remove_urn_uuid(im->id()));
					if (boost::filesystem::is_regular_file(p)) {
						im->read_png_file (p);
					}
				}
			}
		}
		_standard = Standard::SMPTE;
	}

	/* Check that all required image data have been found */
	for (auto i: _subtitles) {
		auto im = dynamic_pointer_cast<SubtitleImage>(i);
		if (im && im->png_image().size() == 0) {
			throw MissingSubtitleImageError (im->id());
		}
	}
}


void
SMPTESubtitleAsset::parse_xml (shared_ptr<cxml::Document> xml)
{
	_xml_id = remove_urn_uuid(xml->string_child("Id"));
	_load_font_nodes = type_children<dcp::SMPTELoadFontNode> (xml, "LoadFont");

	_content_title_text = xml->string_child ("ContentTitleText");
	_annotation_text = xml->optional_string_child ("AnnotationText");
	_issue_date = LocalTime (xml->string_child ("IssueDate"));
	_reel_number = xml->optional_number_child<int> ("ReelNumber");
	_language = xml->optional_string_child ("Language");

	/* This is supposed to be two numbers, but a single number has been seen in the wild */
	auto const er = xml->string_child ("EditRate");
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
		_start_time = Time (xml->string_child("StartTime"), _time_code_rate);
	}

	/* Now we need to drop down to xmlpp */

	vector<ParseState> ps;
	for (auto i: xml->node()->get_children()) {
		auto const e = dynamic_cast<xmlpp::Element const *>(i);
		if (e && e->get_name() == "SubtitleList") {
			parse_subtitles (e, ps, _time_code_rate, Standard::SMPTE);
		}
	}

	/* Guess intrinsic duration */
	_intrinsic_duration = latest_subtitle_out().as_editable_units_ceil(_edit_rate.numerator / _edit_rate.denominator);
}


void
SMPTESubtitleAsset::read_mxf_resources (shared_ptr<ASDCP::TimedText::MXFReader> reader, shared_ptr<DecryptionContext> dec)
{
	ASDCP::TimedText::TimedTextDescriptor descriptor;
	reader->FillTimedTextDescriptor (descriptor);

	/* Load fonts and images */

	for (
		auto i = descriptor.ResourceList.begin();
		i != descriptor.ResourceList.end();
		++i) {

		ASDCP::TimedText::FrameBuffer buffer;
		buffer.Capacity (10 * 1024 * 1024);
		reader->ReadAncillaryResource (i->ResourceID, buffer, dec->context(), dec->hmac());

		char id[64];
		Kumu::bin2UUIDhex (i->ResourceID, ASDCP::UUIDlen, id, sizeof(id));

		shared_array<uint8_t> data (new uint8_t[buffer.Size()]);
		memcpy (data.get(), buffer.RoData(), buffer.Size());

		switch (i->Type) {
		case ASDCP::TimedText::MT_OPENTYPE:
		{
			auto j = _load_font_nodes.begin();
			while (j != _load_font_nodes.end() && (*j)->urn != id) {
				++j;
			}

			if (j != _load_font_nodes.end ()) {
				_fonts.push_back (Font ((*j)->id, (*j)->urn, ArrayData (data, buffer.Size ())));
			}
			break;
		}
		case ASDCP::TimedText::MT_PNG:
		{
			auto j = _subtitles.begin();
			while (j != _subtitles.end() && ((!dynamic_pointer_cast<SubtitleImage>(*j)) || dynamic_pointer_cast<SubtitleImage>(*j)->id() != id)) {
				++j;
			}

			if (j != _subtitles.end()) {
				dynamic_pointer_cast<SubtitleImage>(*j)->set_png_image (ArrayData(data, buffer.Size()));
			}
			break;
		}
		default:
			break;
		}
	}
}


void
SMPTESubtitleAsset::read_mxf_descriptor (shared_ptr<ASDCP::TimedText::MXFReader> reader)
{
	ASDCP::TimedText::TimedTextDescriptor descriptor;
	reader->FillTimedTextDescriptor (descriptor);

	_intrinsic_duration = descriptor.ContainerDuration;
	/* The thing which is called AssetID in the descriptor is also known as the
	 * ResourceID of the MXF.  We store that, at present just for verification
	 * purposes.
	 */
	char id[64];
	Kumu::bin2UUIDhex (descriptor.AssetID, ASDCP::UUIDlen, id, sizeof(id));
	_resource_id = id;
}


void
SMPTESubtitleAsset::set_key (Key key)
{
	/* See if we already have a key; if we do, and we have a file, we'll already
	   have read that file.
	*/
	auto const had_key = static_cast<bool>(_key);

	MXF::set_key (key);

	if (!_key_id || !_file || had_key) {
		/* Either we don't have any data to read, it wasn't
		   encrypted, or we've already read it, so we don't
		   need to do anything else.
		*/
		return;
	}

	/* Our data was encrypted; now we can decrypt it */

	auto reader = make_shared<ASDCP::TimedText::MXFReader>();
	auto r = reader->OpenRead (_file->string().c_str ());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (
			ReadError (
				String::compose ("Could not read encrypted subtitle MXF (%1)", static_cast<int> (r))
				)
			);
	}

	auto dec = make_shared<DecryptionContext>(key, Standard::SMPTE);
	string xml_string;
	reader->ReadTimedTextResource (xml_string, dec->context(), dec->hmac());
	_raw_xml = xml_string;
	auto xml = make_shared<cxml::Document>("SubtitleReel");
	xml->read_string (xml_string);
	parse_xml (xml);
	read_mxf_resources (reader, dec);
}


vector<shared_ptr<LoadFontNode>>
SMPTESubtitleAsset::load_font_nodes () const
{
	vector<shared_ptr<LoadFontNode>> lf;
	copy (_load_font_nodes.begin(), _load_font_nodes.end(), back_inserter(lf));
	return lf;
}


bool
SMPTESubtitleAsset::valid_mxf (boost::filesystem::path file)
{
	ASDCP::TimedText::MXFReader reader;
	Kumu::DefaultLogSink().UnsetFilterFlag(Kumu::LOG_ALLOW_ALL);
	auto r = reader.OpenRead (file.string().c_str ());
	Kumu::DefaultLogSink().SetFilterFlag(Kumu::LOG_ALLOW_ALL);
	return !ASDCP_FAILURE (r);
}


string
SMPTESubtitleAsset::xml_as_string () const
{
	xmlpp::Document doc;
	auto root = doc.create_root_node ("dcst:SubtitleReel");
	root->set_namespace_declaration (subtitle_smpte_ns, "dcst");
	root->set_namespace_declaration ("http://www.w3.org/2001/XMLSchema", "xs");

	DCP_ASSERT (_xml_id);
	root->add_child("Id", "dcst")->add_child_text ("urn:uuid:" + *_xml_id);
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
		root->add_child("StartTime", "dcst")->add_child_text(_start_time.get().as_string(Standard::SMPTE));
	}

	for (auto i: _load_font_nodes) {
		auto load_font = root->add_child("LoadFont", "dcst");
		load_font->add_child_text ("urn:uuid:" + i->urn);
		load_font->set_attribute ("ID", i->id);
	}

	subtitles_as_xml (root->add_child("SubtitleList", "dcst"), _time_code_rate, Standard::SMPTE);

	return doc.write_to_string ("UTF-8");
}


void
SMPTESubtitleAsset::write (boost::filesystem::path p) const
{
	EncryptionContext enc (key(), Standard::SMPTE);

	ASDCP::WriterInfo writer_info;
	fill_writer_info (&writer_info, _id);

	ASDCP::TimedText::TimedTextDescriptor descriptor;
	descriptor.EditRate = ASDCP::Rational (_edit_rate.numerator, _edit_rate.denominator);
	descriptor.EncodingName = "UTF-8";

	/* Font references */

	for (auto i: _load_font_nodes) {
		auto j = _fonts.begin();
		while (j != _fonts.end() && j->load_id != i->id) {
			++j;
		}
		if (j != _fonts.end ()) {
			ASDCP::TimedText::TimedTextResourceDescriptor res;
			unsigned int c;
			Kumu::hex2bin (i->urn.c_str(), res.ResourceID, Kumu::UUID_Length, &c);
			DCP_ASSERT (c == Kumu::UUID_Length);
			res.Type = ASDCP::TimedText::MT_OPENTYPE;
			descriptor.ResourceList.push_back (res);
		}
	}

	/* Image subtitle references */

	for (auto i: _subtitles) {
		auto si = dynamic_pointer_cast<SubtitleImage>(i);
		if (si) {
			ASDCP::TimedText::TimedTextResourceDescriptor res;
			unsigned int c;
			Kumu::hex2bin (si->id().c_str(), res.ResourceID, Kumu::UUID_Length, &c);
			DCP_ASSERT (c == Kumu::UUID_Length);
			res.Type = ASDCP::TimedText::MT_PNG;
			descriptor.ResourceList.push_back (res);
		}
	}

	descriptor.NamespaceName = subtitle_smpte_ns;
	unsigned int c;
	DCP_ASSERT (_xml_id);
	Kumu::hex2bin (_xml_id->c_str(), descriptor.AssetID, ASDCP::UUIDlen, &c);
	DCP_ASSERT (c == Kumu::UUID_Length);
	descriptor.ContainerDuration = _intrinsic_duration;

	ASDCP::TimedText::MXFWriter writer;
	/* This header size is a guess.  Empirically it seems that each subtitle reference is 90 bytes, and we need some extra.
	   The default size is not enough for some feature-length PNG sub projects (see DCP-o-matic #1561).
	*/
	ASDCP::Result_t r = writer.OpenWrite (p.string().c_str(), writer_info, descriptor, _subtitles.size() * 90 + 16384);
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (FileError ("could not open subtitle MXF for writing", p.string(), r));
	}

	_raw_xml = xml_as_string ();

	r = writer.WriteTimedTextResource (*_raw_xml, enc.context(), enc.hmac());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not write XML to timed text resource", p.string(), r));
	}

	/* Font payload */

	for (auto i: _load_font_nodes) {
		auto j = _fonts.begin();
		while (j != _fonts.end() && j->load_id != i->id) {
			++j;
		}
		if (j != _fonts.end ()) {
			ASDCP::TimedText::FrameBuffer buffer;
			ArrayData data_copy(j->data);
			buffer.SetData (data_copy.data(), data_copy.size());
			buffer.Size (j->data.size());
			r = writer.WriteAncillaryResource (buffer, enc.context(), enc.hmac());
			if (ASDCP_FAILURE(r)) {
				boost::throw_exception (MXFFileError ("could not write font to timed text resource", p.string(), r));
			}
		}
	}

	/* Image subtitle payload */

	for (auto i: _subtitles) {
		auto si = dynamic_pointer_cast<SubtitleImage>(i);
		if (si) {
			ASDCP::TimedText::FrameBuffer buffer;
			buffer.SetData (si->png_image().data(), si->png_image().size());
			buffer.Size (si->png_image().size());
			r = writer.WriteAncillaryResource (buffer, enc.context(), enc.hmac());
			if (ASDCP_FAILURE(r)) {
				boost::throw_exception (MXFFileError ("could not write PNG data to timed text resource", p.string(), r));
			}
		}
	}

	writer.Finalize ();

	_file = p;
}

bool
SMPTESubtitleAsset::equals (shared_ptr<const Asset> other_asset, EqualityOptions options, NoteHandler note) const
{
	if (!SubtitleAsset::equals (other_asset, options, note)) {
		return false;
	}

	auto other = dynamic_pointer_cast<const SMPTESubtitleAsset>(other_asset);
	if (!other) {
		note (NoteType::ERROR, "Subtitles are in different standards");
		return false;
	}

	auto i = _load_font_nodes.begin();
	auto j = other->_load_font_nodes.begin();

	while (i != _load_font_nodes.end ()) {
		if (j == other->_load_font_nodes.end ()) {
			note (NoteType::ERROR, "<LoadFont> nodes differ");
			return false;
		}

		if ((*i)->id != (*j)->id) {
			note (NoteType::ERROR, "<LoadFont> nodes differ");
			return false;
		}

		++i;
		++j;
	}

	if (_content_title_text != other->_content_title_text) {
		note (NoteType::ERROR, "Subtitle content title texts differ");
		return false;
	}

	if (_language != other->_language) {
		note (NoteType::ERROR, String::compose("Subtitle languages differ (`%1' vs `%2')", _language.get_value_or("[none]"), other->_language.get_value_or("[none]")));
		return false;
	}

	if (_annotation_text != other->_annotation_text) {
		note (NoteType::ERROR, "Subtitle annotation texts differ");
		return false;
	}

	if (_issue_date != other->_issue_date) {
		if (options.issue_dates_can_differ) {
			note (NoteType::NOTE, "Subtitle issue dates differ");
		} else {
			note (NoteType::ERROR, "Subtitle issue dates differ");
			return false;
		}
	}

	if (_reel_number != other->_reel_number) {
		note (NoteType::ERROR, "Subtitle reel numbers differ");
		return false;
	}

	if (_edit_rate != other->_edit_rate) {
		note (NoteType::ERROR, "Subtitle edit rates differ");
		return false;
	}

	if (_time_code_rate != other->_time_code_rate) {
		note (NoteType::ERROR, "Subtitle time code rates differ");
		return false;
	}

	if (_start_time != other->_start_time) {
		note (NoteType::ERROR, "Subtitle start times differ");
		return false;
	}

	return true;
}


void
SMPTESubtitleAsset::add_font (string load_id, dcp::ArrayData data)
{
	string const uuid = make_uuid ();
	_fonts.push_back (Font(load_id, uuid, data));
	_load_font_nodes.push_back (make_shared<SMPTELoadFontNode>(load_id, uuid));
}


void
SMPTESubtitleAsset::add (shared_ptr<Subtitle> s)
{
	SubtitleAsset::add (s);
	_intrinsic_duration = latest_subtitle_out().as_editable_units_ceil(_edit_rate.numerator / _edit_rate.denominator);
}

