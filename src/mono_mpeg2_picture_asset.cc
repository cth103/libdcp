/*
    Copyright (C) 2023 Carl Hetherington <cth@carlh.net>

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


#include "filesystem.h"
#include "mono_mpeg2_picture_asset.h"
#include "mono_mpeg2_picture_asset_reader.h"
#include <asdcp/AS_DCP.h>


using std::shared_ptr;
using namespace dcp;


MonoMPEG2PictureAsset::MonoMPEG2PictureAsset(boost::filesystem::path file)
	: MPEG2PictureAsset(file)
{
	Kumu::FileReaderFactory factory;
	ASDCP::MPEG2::MXFReader reader(factory);
	auto const result = reader.OpenRead(dcp::filesystem::fix_long_path(file).string().c_str());
	if (ASDCP_FAILURE(result)) {
		boost::throw_exception(MXFFileError("could not open MXF file for reading", file.string(), result));
	}

	ASDCP::MPEG2::VideoDescriptor desc;
	if (ASDCP_FAILURE(reader.FillVideoDescriptor(desc))) {
		boost::throw_exception(ReadError("could not read video MXF information"));
	}

	read_video_descriptor(desc);

	ASDCP::WriterInfo info;
	if (ASDCP_FAILURE(reader.FillWriterInfo(info))) {
		boost::throw_exception(ReadError("could not read video MXF information"));
	}

	_id = read_writer_info(info);
}


shared_ptr<MonoMPEG2PictureAssetReader>
MonoMPEG2PictureAsset::start_read () const
{
	/* Can't use make_shared here as the MonoMPEG2PictureAssetReader constructor is private */
	return shared_ptr<MonoMPEG2PictureAssetReader>(new MonoMPEG2PictureAssetReader(this, key(), standard()));

}
