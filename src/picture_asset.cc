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


/** @file  src/picture_asset.cc
 *  @brief PictureAsset class
 */


#include "picture_asset.h"
#include "util.h"
#include "exceptions.h"
#include "openjpeg_image.h"
#include "picture_asset_writer.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include "j2k_transcode.h"
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_fileio.h>
#include <libxml++/nodes/element.h>
#include <boost/filesystem.hpp>
#include <list>
#include <stdexcept>


using std::string;
using std::list;
using std::vector;
using std::max;
using std::pair;
using std::make_pair;
using std::shared_ptr;
using namespace dcp;


PictureAsset::PictureAsset (boost::filesystem::path file)
	: Asset (file)
	, _intrinsic_duration (0)
{

}


PictureAsset::PictureAsset (Fraction edit_rate, Standard standard)
	: MXF (standard)
	, _edit_rate (edit_rate)
	, _intrinsic_duration (0)
{

}


void
PictureAsset::read_picture_descriptor (ASDCP::JP2K::PictureDescriptor const & desc)
{
	_size.width = desc.StoredWidth;
	_size.height = desc.StoredHeight;
	_edit_rate = Fraction (desc.EditRate.Numerator, desc.EditRate.Denominator);
	_intrinsic_duration = desc.ContainerDuration;
	_frame_rate = Fraction (desc.SampleRate.Numerator, desc.SampleRate.Denominator);
	_screen_aspect_ratio = Fraction (desc.AspectRatio.Numerator, desc.AspectRatio.Denominator);
}


bool
PictureAsset::descriptor_equals (
	ASDCP::JP2K::PictureDescriptor const & a, ASDCP::JP2K::PictureDescriptor const & b, NoteHandler note
	) const
{
	if (
		a.EditRate != b.EditRate ||
		a.SampleRate != b.SampleRate ||
		a.StoredWidth != b.StoredWidth ||
		a.StoredHeight != b.StoredHeight ||
		a.AspectRatio != b.AspectRatio ||
		a.Rsize != b.Rsize ||
		a.Xsize != b.Xsize ||
		a.Ysize != b.Ysize ||
		a.XOsize != b.XOsize ||
		a.YOsize != b.YOsize ||
		a.XTsize != b.XTsize ||
		a.YTsize != b.YTsize ||
		a.XTOsize != b.XTOsize ||
		a.YTOsize != b.YTOsize ||
		a.Csize != b.Csize
//		a.CodingStyleDefault != b.CodingStyleDefault ||
//		a.QuantizationDefault != b.QuantizationDefault
		) {

		note (NoteType::ERROR, "video MXF picture descriptors differ");
		return false;
	}

	if (a.ContainerDuration != b.ContainerDuration) {
		note (NoteType::ERROR, "video container durations differ");
	}

//		for (unsigned int j = 0; j < ASDCP::JP2K::MaxComponents; ++j) {
//			if (a.ImageComponents[j] != b.ImageComponents[j]) {
//				notes.pack_start ("video MXF picture descriptors differ");
//			}
//		}

	return true;
}


bool
PictureAsset::frame_buffer_equals (
	int frame, EqualityOptions opt, NoteHandler note,
	uint8_t const * data_A, unsigned int size_A, uint8_t const * data_B, unsigned int size_B
	) const
{
	if (size_A == size_B && memcmp (data_A, data_B, size_A) == 0) {
		note (NoteType::NOTE, "J2K identical");
		/* Easy result; the J2K data is identical */
		return true;
	}

	/* Decompress the images to bitmaps */
	auto image_A = decompress_j2k (const_cast<uint8_t*>(data_A), size_A, 0);
	auto image_B = decompress_j2k (const_cast<uint8_t*>(data_B), size_B, 0);

	/* Compare them */

	vector<int> abs_diffs (image_A->size().width * image_A->size().height * 3);
	int d = 0;
	int max_diff = 0;

	for (int c = 0; c < 3; ++c) {

		if (image_A->size() != image_B->size()) {
			note (NoteType::ERROR, String::compose ("image sizes for frame %1 differ", frame));
			return false;
		}

		int const pixels = image_A->size().width * image_A->size().height;
		for (int j = 0; j < pixels; ++j) {
			int const t = abs (image_A->data(c)[j] - image_B->data(c)[j]);
			abs_diffs[d++] = t;
			max_diff = max (max_diff, t);
		}
	}

	uint64_t total = 0;
	for (vector<int>::iterator j = abs_diffs.begin(); j != abs_diffs.end(); ++j) {
		total += *j;
	}

	double const mean = double (total) / abs_diffs.size ();

	uint64_t total_squared_deviation = 0;
	for (auto j: abs_diffs) {
		total_squared_deviation += pow (j - mean, 2);
	}

	auto const std_dev = sqrt (double (total_squared_deviation) / abs_diffs.size());

	note (NoteType::NOTE, String::compose("mean difference %1 deviation %2", mean, std_dev));

	if (mean > opt.max_mean_pixel_error) {
		note (
			NoteType::ERROR,
			String::compose ("mean %1 out of range %2 in frame %3", mean, opt.max_mean_pixel_error, frame)
			);

		return false;
	}

	if (std_dev > opt.max_std_dev_pixel_error) {
		note (
			NoteType::ERROR,
			String::compose ("standard deviation %1 out of range %2 in frame %3", std_dev, opt.max_std_dev_pixel_error, frame)
			);

		return false;
	}

	return true;
}


string
PictureAsset::static_pkl_type (Standard standard)
{
	switch (standard) {
	case Standard::INTEROP:
		return "application/x-smpte-mxf;asdcpKind=Picture";
	case Standard::SMPTE:
		return "application/mxf";
	default:
		DCP_ASSERT (false);
	}
}


string
PictureAsset::pkl_type (Standard standard) const
{
	return static_pkl_type (standard);
}
