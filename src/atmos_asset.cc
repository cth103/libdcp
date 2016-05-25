/*
    Copyright (C) 2016 Carl Hetherington <cth@carlh.net>

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

#include "atmos_asset.h"
#include "exceptions.h"
#include "AS_DCP.h"

using std::string;
using namespace dcp;

AtmosAsset::AtmosAsset (boost::filesystem::path file)
	: Asset (file)
{
	ASDCP::ATMOS::MXFReader reader;
	Kumu::Result_t r = reader.OpenRead (file.string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", file.string(), r));
	}

	ASDCP::ATMOS::AtmosDescriptor desc;
	if (ASDCP_FAILURE (reader.FillAtmosDescriptor (desc))) {
		boost::throw_exception (DCPReadError ("could not read Atmos MXF information"));
	}

	_edit_rate = Fraction (desc.EditRate.Numerator, desc.EditRate.Denominator);
	_intrinsic_duration = desc.ContainerDuration;
	_first_frame = desc.FirstFrame;
	_max_channel_count = desc.MaxChannelCount;
	_max_object_count = desc.MaxObjectCount;
}

string
AtmosAsset::pkl_type (Standard) const
{
	return "application/mxf";
}
