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

using std::string;
using std::list;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using namespace libdcp;

MXFAsset::MXFAsset (string directory, string file_name)
	: Asset (directory, file_name)
	, _progress (0)
	, _edit_rate (0)
	, _entry_point (0)
	, _intrinsic_duration (0)
	, _duration (0)
{

}

MXFAsset::MXFAsset (string directory, string file_name, boost::signals2::signal<void (float)>* progress, int edit_rate, int intrinsic_duration)
	: Asset (directory, file_name)
	, _progress (progress)
	, _edit_rate (edit_rate)
	, _entry_point (0)
	, _intrinsic_duration (intrinsic_duration)
	, _duration (intrinsic_duration)
{
	
}
void
MXFAsset::fill_writer_info (ASDCP::WriterInfo* writer_info, string uuid)
{
	writer_info->ProductVersion = Metadata::instance()->product_version;
	writer_info->CompanyName = Metadata::instance()->company_name;
	writer_info->ProductName = Metadata::instance()->product_name.c_str();

	writer_info->LabelSetType = ASDCP::LS_MXF_SMPTE;
	unsigned int c;
	Kumu::hex2bin (uuid.c_str(), writer_info->AssetUUID, Kumu::UUID_Length, &c);
	assert (c == Kumu::UUID_Length);
}

bool
MXFAsset::equals (shared_ptr<const Asset> other, EqualityOptions, list<string>& notes) const
{
	shared_ptr<const MXFAsset> other_mxf = dynamic_pointer_cast<const MXFAsset> (other);
	if (!other_mxf) {
		notes.push_back ("comparing an MXF asset with a non-MXF asset");
		return false;
	}
	
	if (_file_name != other_mxf->_file_name) {
		notes.push_back ("MXF names differ");
		return false;
	}

	if (_edit_rate != other_mxf->_edit_rate) {
		notes.push_back ("MXF edit rates differ");
		return false;
	}
	
	if (_intrinsic_duration != other_mxf->_intrinsic_duration) {
		notes.push_back ("MXF intrinsic durations differ");
		return false;
	}

	if (_duration != other_mxf->_duration) {
		notes.push_back ("MXF durations differ");
		return false;
	}
	
	return true;
}
