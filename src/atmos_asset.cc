/*
    Copyright (C) 2016 Carl Hetherington <cth@carlh.net>

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

#include "atmos_asset.h"
#include "atmos_asset_reader.h"
#include "atmos_asset_writer.h"
#include "exceptions.h"
#include <asdcp/AS_DCP.h>

using std::string;
using boost::shared_ptr;
using namespace dcp;

AtmosAsset::AtmosAsset (Fraction edit_rate, int first_frame, int max_channel_count, int max_object_count, string atmos_id, int atmos_version)
	: MXF (SMPTE)
	, _edit_rate (edit_rate)
	, _intrinsic_duration (0)
	, _first_frame (first_frame)
	, _max_channel_count (max_channel_count)
	, _max_object_count (max_object_count)
	, _atmos_id (atmos_id)
	, _atmos_version (atmos_version)
{

}

AtmosAsset::AtmosAsset (boost::filesystem::path file)
	: Asset (file)
	, MXF (SMPTE)
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

	char id[64];
	Kumu::bin2UUIDhex (desc.AtmosID, ASDCP::UUIDlen, id, sizeof (id));
	_atmos_id = id;

	_atmos_version = desc.AtmosVersion;
}

string
AtmosAsset::pkl_type (Standard) const
{
	return "application/mxf";
}

shared_ptr<AtmosAssetReader>
AtmosAsset::start_read () const
{
	return shared_ptr<AtmosAssetReader> (new AtmosAssetReader (this, key ()));
}

shared_ptr<AtmosAssetWriter>
AtmosAsset::start_write (boost::filesystem::path file)
{
	return shared_ptr<AtmosAssetWriter> (new AtmosAssetWriter (this, file));
}
