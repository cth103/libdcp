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

/** @file  src/picture_asset.cc
 *  @brief An asset made up of JPEG2000 files
 */

#include <list>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <openjpeg.h>
#include "AS_DCP.h"
#include "KM_fileio.h"
#include "picture_asset.h"
#include "util.h"
#include "exceptions.h"
#include "picture_frame.h"

using std::string;
using std::ostream;
using std::list;
using std::vector;
using std::max;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using boost::lexical_cast;
using namespace libdcp;

PictureAsset::PictureAsset (string directory, string mxf_name, boost::signals2::signal<void (float)>* progress, int fps, int intrinsic_duration)
	: MXFAsset (directory, mxf_name, progress, fps, intrinsic_duration)
{

}

void
PictureAsset::write_to_cpl (ostream& s) const
{
	s << "        <MainPicture>\n"
	  << "          <Id>urn:uuid:" << _uuid << "</Id>\n"
	  << "          <AnnotationText>" << _file_name << "</AnnotationText>\n"
	  << "          <EditRate>" << _fps << " 1</EditRate>\n"
	  << "          <IntrinsicDuration>" << _intrinsic_duration << "</IntrinsicDuration>\n"
	  << "          <EntryPoint>0</EntryPoint>\n"
	  << "          <Duration>" << _intrinsic_duration << "</Duration>\n"
	  << "          <FrameRate>" << _fps << " 1</FrameRate>\n"
	  << "          <ScreenAspectRatio>" << _size.width << " " << _size.height << "</ScreenAspectRatio>\n"
	  << "        </MainPicture>\n";
}

bool
PictureAsset::equals (shared_ptr<const Asset> other, EqualityOptions opt, list<string>& notes) const
{
	if (!MXFAsset::equals (other, opt, notes)) {
		return false;
	}
		     
	ASDCP::JP2K::MXFReader reader_A;
	if (ASDCP_FAILURE (reader_A.OpenRead (path().string().c_str()))) {
		throw MXFFileError ("could not open MXF file for reading", path().string());
	}
	
	ASDCP::JP2K::MXFReader reader_B;
	if (ASDCP_FAILURE (reader_B.OpenRead (other->path().string().c_str()))) {
		throw MXFFileError ("could not open MXF file for reading", path().string());
	}
	
	ASDCP::JP2K::PictureDescriptor desc_A;
	if (ASDCP_FAILURE (reader_A.FillPictureDescriptor (desc_A))) {
		throw DCPReadError ("could not read video MXF information");
	}
	ASDCP::JP2K::PictureDescriptor desc_B;
	if (ASDCP_FAILURE (reader_B.FillPictureDescriptor (desc_B))) {
		throw DCPReadError ("could not read video MXF information");
	}
	
	if (
		desc_A.EditRate != desc_B.EditRate ||
		desc_A.ContainerDuration != desc_B.ContainerDuration ||
		desc_A.SampleRate != desc_B.SampleRate ||
		desc_A.StoredWidth != desc_B.StoredWidth ||
		desc_A.StoredHeight != desc_B.StoredHeight ||
		desc_A.AspectRatio != desc_B.AspectRatio ||
		desc_A.Rsize != desc_B.Rsize ||
		desc_A.Xsize != desc_B.Xsize ||
		desc_A.Ysize != desc_B.Ysize ||
		desc_A.XOsize != desc_B.XOsize ||
		desc_A.YOsize != desc_B.YOsize ||
		desc_A.XTsize != desc_B.XTsize ||
		desc_A.YTsize != desc_B.YTsize ||
		desc_A.XTOsize != desc_B.XTOsize ||
		desc_A.YTOsize != desc_B.YTOsize ||
		desc_A.Csize != desc_B.Csize
//		desc_A.CodingStyleDefault != desc_B.CodingStyleDefault ||
//		desc_A.QuantizationDefault != desc_B.QuantizationDefault
		) {
		
		notes.push_back ("video MXF picture descriptors differ");
		return false;
	}

//		for (unsigned int j = 0; j < ASDCP::JP2K::MaxComponents; ++j) {
//			if (desc_A.ImageComponents[j] != desc_B.ImageComponents[j]) {
//				notes.pack_start ("video MXF picture descriptors differ");
//			}
//		}

	return true;
}


MonoPictureAsset::MonoPictureAsset (
	boost::function<string (int)> get_path,
	string directory,
	string mxf_name,
	boost::signals2::signal<void (float)>* progress,
	int fps,
	int intrinsic_duration,
	Size size)
	: PictureAsset (directory, mxf_name, progress, fps, intrinsic_duration)
{
	_size = size;
	construct (get_path);
}

MonoPictureAsset::MonoPictureAsset (
	vector<string> const & files,
	string directory,
	string mxf_name,
	boost::signals2::signal<void (float)>* progress,
	int fps,
	int intrinsic_duration,
	Size size)
	: PictureAsset (directory, mxf_name, progress, fps, intrinsic_duration)
{
	_size = size;
	construct (boost::bind (&MonoPictureAsset::path_from_list, this, _1, files));
}

MonoPictureAsset::MonoPictureAsset (string directory, string mxf_name, int fps, int intrinsic_duration)
	: PictureAsset (directory, mxf_name, 0, fps, intrinsic_duration)
{
	ASDCP::JP2K::MXFReader reader;
	if (ASDCP_FAILURE (reader.OpenRead (path().string().c_str()))) {
		throw MXFFileError ("could not open MXF file for reading", path().string());
	}
	
	ASDCP::JP2K::PictureDescriptor desc;
	if (ASDCP_FAILURE (reader.FillPictureDescriptor (desc))) {
		throw DCPReadError ("could not read video MXF information");
	}

	_size.width = desc.StoredWidth;
	_size.height = desc.StoredHeight;
}

void
MonoPictureAsset::construct (boost::function<string (int)> get_path)
{
	ASDCP::JP2K::CodestreamParser j2k_parser;
	ASDCP::JP2K::FrameBuffer frame_buffer (4 * Kumu::Megabyte);
	if (ASDCP_FAILURE (j2k_parser.OpenReadFrame (get_path(0).c_str(), frame_buffer))) {
		throw FileError ("could not open JPEG2000 file for reading", get_path (0));
	}
	
	ASDCP::JP2K::PictureDescriptor picture_desc;
	j2k_parser.FillPictureDescriptor (picture_desc);
	picture_desc.EditRate = ASDCP::Rational (_fps, 1);
	
	ASDCP::WriterInfo writer_info;
	fill_writer_info (&writer_info);
	
	ASDCP::JP2K::MXFWriter mxf_writer;
	if (ASDCP_FAILURE (mxf_writer.OpenWrite (path().string().c_str(), writer_info, picture_desc))) {
		throw MXFFileError ("could not open MXF file for writing", path().string());
	}

	for (int i = 0; i < _intrinsic_duration; ++i) {

		string const path = get_path (i);

		if (ASDCP_FAILURE (j2k_parser.OpenReadFrame (path.c_str(), frame_buffer))) {
			throw FileError ("could not open JPEG2000 file for reading", path);
		}

		/* XXX: passing 0 to WriteFrame ok? */
		if (ASDCP_FAILURE (mxf_writer.WriteFrame (frame_buffer, 0, 0))) {
			throw MiscError ("error in writing video MXF");
		}

		if (_progress) {
			(*_progress) (0.5 * float (i) / _intrinsic_duration);
		}
	}
	
	if (ASDCP_FAILURE (mxf_writer.Finalize())) {
		throw MiscError ("error in finalising video MXF");
	}
}

string
MonoPictureAsset::path_from_list (int f, vector<string> const & files) const
{
	return files[f];
}

shared_ptr<const MonoPictureFrame>
MonoPictureAsset::get_frame (int n) const
{
	return shared_ptr<const MonoPictureFrame> (new MonoPictureFrame (path().string(), n + _entry_point));
}


bool
MonoPictureAsset::equals (shared_ptr<const Asset> other, EqualityOptions opt, list<string>& notes) const
{
	if (!PictureAsset::equals (other, opt, notes)) {
		return false;
	}

	shared_ptr<const MonoPictureAsset> other_picture = dynamic_pointer_cast<const MonoPictureAsset> (other);
	assert (other_picture);

	for (int i = 0; i < _intrinsic_duration; ++i) {
		shared_ptr<const MonoPictureFrame> frame_A = get_frame (i);
		shared_ptr<const MonoPictureFrame> frame_B = other_picture->get_frame (i);
		
		if (!frame_buffer_equals (
			    i, opt, notes,
			    frame_A->j2k_frame()->RoData(), frame_A->j2k_frame()->Size(),
			    frame_B->j2k_frame()->RoData(), frame_B->j2k_frame()->Size()
			    )) {
			return false;
		}
	}

	return true;
}

bool
StereoPictureAsset::equals (shared_ptr<const Asset> other, EqualityOptions opt, list<string>& notes) const
{
	if (!PictureAsset::equals (other, opt, notes)) {
		return false;
	}
	
	shared_ptr<const StereoPictureAsset> other_picture = dynamic_pointer_cast<const StereoPictureAsset> (other);
	assert (other_picture);

	for (int i = 0; i < _intrinsic_duration; ++i) {
		shared_ptr<const StereoPictureFrame> frame_A = get_frame (i);
		shared_ptr<const StereoPictureFrame> frame_B = other_picture->get_frame (i);
		
		if (!frame_buffer_equals (
			    i, opt, notes,
			    frame_A->j2k_frame()->Left.RoData(), frame_A->j2k_frame()->Left.Size(),
			    frame_B->j2k_frame()->Left.RoData(), frame_B->j2k_frame()->Left.Size()
			    )) {
			return false;
		}
		
		if (!frame_buffer_equals (
			    i, opt, notes,
			    frame_A->j2k_frame()->Right.RoData(), frame_A->j2k_frame()->Right.Size(),
			    frame_B->j2k_frame()->Right.RoData(), frame_B->j2k_frame()->Right.Size()
			    )) {
			return false;
		}
	}

	return true;
}

bool
PictureAsset::frame_buffer_equals (
	int frame, EqualityOptions opt, list<string>& notes, uint8_t const * data_A, unsigned int size_A, uint8_t const * data_B, unsigned int size_B
	) const
{
	if (size_A == size_B && memcmp (data_A, data_B, size_A) == 0) {
		/* Easy result; the J2K data is identical */
		return true;
	}
		
	/* Decompress the images to bitmaps */
	opj_image_t* image_A = decompress_j2k (const_cast<uint8_t*> (data_A), size_A, 0);
	opj_image_t* image_B = decompress_j2k (const_cast<uint8_t*> (data_B), size_B, 0);
	
	/* Compare them */
	
	if (image_A->numcomps != image_B->numcomps) {
		notes.push_back ("image component counts for frame " + lexical_cast<string>(frame) + " differ");
		return false;
	}
	
	vector<int> abs_diffs (image_A->comps[0].w * image_A->comps[0].h * image_A->numcomps);
	int d = 0;
	int max_diff = 0;
	
	for (int c = 0; c < image_A->numcomps; ++c) {
		
		if (image_A->comps[c].w != image_B->comps[c].w || image_A->comps[c].h != image_B->comps[c].h) {
			notes.push_back ("image sizes for frame " + lexical_cast<string>(frame) + " differ");
			return false;
		}
		
		int const pixels = image_A->comps[c].w * image_A->comps[c].h;
		for (int j = 0; j < pixels; ++j) {
			int const t = abs (image_A->comps[c].data[j] - image_B->comps[c].data[j]);
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
	
	if (mean > opt.max_mean_pixel_error || std_dev > opt.max_std_dev_pixel_error) {
		notes.push_back ("mean or standard deviation out of range for " + lexical_cast<string>(frame));
		return false;
	}
	
	opj_image_destroy (image_A);
	opj_image_destroy (image_B);

	return true;
}


StereoPictureAsset::StereoPictureAsset (string directory, string mxf_name, int fps, int intrinsic_duration)
	: PictureAsset (directory, mxf_name, 0, fps, intrinsic_duration)
{
	ASDCP::JP2K::MXFSReader reader;
	if (ASDCP_FAILURE (reader.OpenRead (path().string().c_str()))) {
		throw MXFFileError ("could not open MXF file for reading", path().string());
	}
	
	ASDCP::JP2K::PictureDescriptor desc;
	if (ASDCP_FAILURE (reader.FillPictureDescriptor (desc))) {
		throw DCPReadError ("could not read video MXF information");
	}

	_size.width = desc.StoredWidth;
	_size.height = desc.StoredHeight;
}

shared_ptr<const StereoPictureFrame>
StereoPictureAsset::get_frame (int n) const
{
	return shared_ptr<const StereoPictureFrame> (new StereoPictureFrame (path().string(), n + _entry_point));
}
