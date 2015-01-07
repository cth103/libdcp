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

#include "picture_mxf.h"
#include "util.h"
#include "exceptions.h"
#include "xyz_frame.h"
#include "picture_mxf_writer.h"
#include "compose.hpp"
#include "AS_DCP.h"
#include "KM_fileio.h"
#include <libxml++/nodes/element.h>
#include <openjpeg.h>
#include <boost/filesystem.hpp>
#include <list>
#include <stdexcept>
#include <iostream>
#include <sstream>

using std::string;
using std::ostream;
using std::list;
using std::vector;
using std::max;
using std::stringstream;
using std::pair;
using std::make_pair;
using std::istream;
using std::cout;
using boost::shared_ptr;
using namespace dcp;

PictureMXF::PictureMXF (boost::filesystem::path file)
	: MXF (file)
{

}

PictureMXF::PictureMXF (Fraction edit_rate)
	: MXF (edit_rate)
{

}

void
PictureMXF::read_picture_descriptor (ASDCP::JP2K::PictureDescriptor const & desc)
{
	_size.width = desc.StoredWidth;
	_size.height = desc.StoredHeight;
	_edit_rate = Fraction (desc.EditRate.Numerator, desc.EditRate.Denominator);
	_intrinsic_duration = desc.ContainerDuration;
	_frame_rate = Fraction (desc.SampleRate.Numerator, desc.SampleRate.Denominator);
	_screen_aspect_ratio = Fraction (desc.AspectRatio.Numerator, desc.AspectRatio.Denominator);
}

bool
PictureMXF::descriptor_equals (
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
		
		note (DCP_ERROR, "video MXF picture descriptors differ");
		return false;
	}

	if (a.ContainerDuration != b.ContainerDuration) {
		note (DCP_ERROR, "video container durations differ");
	}
	
//		for (unsigned int j = 0; j < ASDCP::JP2K::MaxComponents; ++j) {
//			if (a.ImageComponents[j] != b.ImageComponents[j]) {
//				notes.pack_start ("video MXF picture descriptors differ");
//			}
//		}

	return true;
}

bool
PictureMXF::frame_buffer_equals (
	int frame, EqualityOptions opt, NoteHandler note,
	uint8_t const * data_A, unsigned int size_A, uint8_t const * data_B, unsigned int size_B
	) const
{
	if (size_A == size_B && memcmp (data_A, data_B, size_A) == 0) {
		note (DCP_NOTE, "J2K identical");
		/* Easy result; the J2K data is identical */
		return true;
	}
		
	/* Decompress the images to bitmaps */
	shared_ptr<XYZFrame> image_A = decompress_j2k (const_cast<uint8_t*> (data_A), size_A, 0);
	shared_ptr<XYZFrame> image_B = decompress_j2k (const_cast<uint8_t*> (data_B), size_B, 0);
	
	/* Compare them */
	
	vector<int> abs_diffs (image_A->size().width * image_A->size().height * 3);
	int d = 0;
	int max_diff = 0;
	
	for (int c = 0; c < 3; ++c) {
		
		if (image_A->size() != image_B->size()) {
			note (DCP_ERROR, String::compose ("image sizes for frame %1 differ", frame));
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
	for (vector<int>::iterator j = abs_diffs.begin(); j != abs_diffs.end(); ++j) {
		total_squared_deviation += pow (*j - mean, 2);
	}
	
	double const std_dev = sqrt (double (total_squared_deviation) / abs_diffs.size());
	
	note (DCP_NOTE, String::compose ("mean difference %1, deviation %2", mean, std_dev));
	
	if (mean > opt.max_mean_pixel_error) {
		note (
			DCP_ERROR,
			String::compose ("mean %1 out of range %2 in frame %3", mean, opt.max_mean_pixel_error, frame)
			);
		
		return false;
	}

	if (std_dev > opt.max_std_dev_pixel_error) {
		note (
			DCP_ERROR,
			String::compose ("standard deviation %1 out of range %2 in frame %3", std_dev, opt.max_std_dev_pixel_error, frame)
			);
		
		return false;
	}

	return true;
}
