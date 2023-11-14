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


#include "mpeg2_picture_asset.h"


using std::string;
using namespace dcp;


MPEG2PictureAsset::MPEG2PictureAsset(boost::filesystem::path file)
	: PictureAsset(file)
{

}


void
MPEG2PictureAsset::read_video_descriptor(ASDCP::MPEG2::VideoDescriptor const& descriptor)
{
	_size.width = descriptor.StoredWidth;
	_size.height = descriptor.StoredHeight;
	_edit_rate = Fraction(descriptor.EditRate.Numerator, descriptor.EditRate.Denominator);
	_intrinsic_duration = descriptor.ContainerDuration;
	_frame_rate = Fraction(descriptor.SampleRate.Numerator, descriptor.SampleRate.Denominator);
	_screen_aspect_ratio = Fraction(descriptor.AspectRatio.Numerator, descriptor.AspectRatio.Denominator);
}


string
MPEG2PictureAsset::pkl_type (Standard standard) const
{
	DCP_ASSERT(standard == Standard::INTEROP);
	return "application/x-smpte-mxf;asdcpKind=Picture";
}


string
MPEG2PictureAsset::static_pkl_type(Standard standard)
{
	DCP_ASSERT(standard == Standard::INTEROP);
	return "application/x-smpte-mxf;asdcpKind=Picture";
}
