/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

/** @file  src/asset.cc
 *  @brief Parent class for assets of DCPs made up of MXF files.
 */

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include "AS_DCP.h"
#include "KM_util.h"
#include "mxf_asset.h"
#include "util.h"
#include "metadata.h"

using namespace std;
using namespace boost;
using namespace libdcp;

MXFAsset::MXFAsset (string directory, string file_name, sigc::signal1<void, float>* progress, int fps, int length)
	: Asset (directory, file_name)
	, _progress (progress)
	, _fps (fps)
	, _length (length)
{
	
}

void
MXFAsset::fill_writer_info (ASDCP::WriterInfo* writer_info) const
{
	writer_info->ProductVersion = Metadata::instance()->product_version;
	writer_info->CompanyName = Metadata::instance()->company_name;
	writer_info->ProductName = Metadata::instance()->product_name.c_str();

	writer_info->LabelSetType = ASDCP::LS_MXF_SMPTE;
	unsigned int c;
	Kumu::hex2bin (_uuid.c_str(), writer_info->AssetUUID, Kumu::UUID_Length, &c);
	assert (c == Kumu::UUID_Length);
}

list<string>
MXFAsset::equals (shared_ptr<const Asset> other, EqualityOptions opt) const
{
	shared_ptr<const MXFAsset> other_mxf = dynamic_pointer_cast<const MXFAsset> (other);
	if (!other_mxf) {
		return list<string> ();
	}
	
	list<string> notes;
	
	if (opt.flags & LIBDCP_METADATA) {
		if (_file_name != other_mxf->_file_name) {
			notes.push_back ("MXF names differ");
		}
		if (_fps != other_mxf->_fps) {
			notes.push_back ("MXF frames per second differ");
		}
		if (_length != other_mxf->_length) {
			notes.push_back ("MXF lengths differ");
		}
	}
	
	if (opt.flags & MXF_BITWISE) {

		if (digest() != other_mxf->digest()) {
			notes.push_back ("MXF digests differ");
		}
		
		if (filesystem::file_size (path()) != filesystem::file_size (other_mxf->path())) {
			notes.push_back (path().string() + " and " + other_mxf->path().string() + " sizes differ");
			return notes;
		}
		
		ifstream a (path().string().c_str(), ios::binary);
		ifstream b (other_mxf->path().string().c_str(), ios::binary);

		int buffer_size = 65536;
		char abuffer[buffer_size];
		char bbuffer[buffer_size];

		int n = filesystem::file_size (path ());

		while (n) {
			int const t = min (n, buffer_size);
			a.read (abuffer, t);
			b.read (bbuffer, t);

			if (memcmp (abuffer, bbuffer, t) != 0) {
				notes.push_back (path().string() + " and " + other_mxf->path().string() + " content differs");
				return notes;
			}

			n -= t;
		}
	}

	return notes;
}

int
MXFAsset::length () const
{
	return _length;
}

	
