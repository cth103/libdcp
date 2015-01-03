/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#include "smpte_subtitle_content.h"
#include "smpte_load_font.h"
#include "font.h"
#include "exceptions.h"
#include "xml.h"
#include "AS_DCP.h"
#include "KM_util.h"

using std::string;
using std::list;
using std::stringstream;
using boost::shared_ptr;
using namespace dcp;

SMPTESubtitleContent::SMPTESubtitleContent (boost::filesystem::path file, bool mxf)
	: SubtitleContent (file)
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
	
	_load_font_nodes = type_children<dcp::SMPTELoadFont> (xml, "LoadFont");

	shared_ptr<cxml::Node> subtitle_list = xml->optional_node_child ("SubtitleList");
	list<shared_ptr<dcp::Font> > font_nodes = type_children<dcp::Font> (subtitle_list, "Font");

	parse_common (xml, font_nodes);
}
