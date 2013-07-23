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
#include <libxml++/nodes/element.h>
#include "AS_DCP.h"
#include "KM_fileio.h"
#include "picture_asset.h"
#include "util.h"
#include "exceptions.h"
#include "picture_frame.h"
#include "xyz_frame.h"
#include "picture_asset_writer.h"

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
using boost::dynamic_pointer_cast;
using boost::lexical_cast;
using namespace libdcp;

PictureAsset::PictureAsset (string directory, string mxf_name, boost::signals2::signal<void (float)>* progress, int fps, int intrinsic_duration, bool encrypted, Size size)
	: MXFAsset (directory, mxf_name, progress, fps, intrinsic_duration, encrypted)
	, _size (size)
{

}

PictureAsset::PictureAsset (string directory, string mxf_name)
	: MXFAsset (directory, mxf_name)
{

}

string
MonoPictureAsset::cpl_node_name () const
{
	return "MainPicture";
}

int
MonoPictureAsset::edit_rate_factor () const
{
	return 1;
}

string
StereoPictureAsset::cpl_node_name () const
{
	return "MainStereoscopicPicture";
}

int
StereoPictureAsset::edit_rate_factor () const
{
	return 2;
}

void
PictureAsset::write_to_cpl (xmlpp::Node* node) const
{
	MXFAsset::write_to_cpl (node);
	
	xmlpp::Node::NodeList c = node->get_children ();
	xmlpp::Node::NodeList::iterator i = c.begin();
	while (i != c.end() && (*i)->get_name() != cpl_node_name ()) {
		++i;
	}

	assert (i != c.end ());

	(*i)->add_child ("FrameRate")->add_child_text (lexical_cast<string> (_edit_rate * edit_rate_factor ()) + " 1");
	(*i)->add_child ("ScreenAspectRatio")->add_child_text (lexical_cast<string> (_size.width) + " " + lexical_cast<string> (_size.height));
}

bool
PictureAsset::equals (shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (!MXFAsset::equals (other, opt, note)) {
		return false;
	}
		     
	ASDCP::JP2K::MXFReader reader_A;
	if (ASDCP_FAILURE (reader_A.OpenRead (path().string().c_str()))) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", path().string()));
	}
	
	ASDCP::JP2K::MXFReader reader_B;
	if (ASDCP_FAILURE (reader_B.OpenRead (other->path().string().c_str()))) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", path().string()));
	}
	
	ASDCP::JP2K::PictureDescriptor desc_A;
	if (ASDCP_FAILURE (reader_A.FillPictureDescriptor (desc_A))) {
		boost::throw_exception (DCPReadError ("could not read video MXF information"));
	}
	ASDCP::JP2K::PictureDescriptor desc_B;
	if (ASDCP_FAILURE (reader_B.FillPictureDescriptor (desc_B))) {
		boost::throw_exception (DCPReadError ("could not read video MXF information"));
	}
	
	if (
		desc_A.EditRate != desc_B.EditRate ||
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
		
		note (ERROR, "video MXF picture descriptors differ");
		return false;
	}

	if (desc_A.ContainerDuration != desc_B.ContainerDuration) {
		note (ERROR, "video container durations differ");
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
	bool encrypted,
	Size size,
	MXFMetadata const & metadata
	)
	: PictureAsset (directory, mxf_name, progress, fps, intrinsic_duration, encrypted, size)
{
	construct (get_path, metadata);
}

MonoPictureAsset::MonoPictureAsset (
	vector<string> const & files,
	string directory,
	string mxf_name,
	boost::signals2::signal<void (float)>* progress,
	int fps,
	int intrinsic_duration,
	bool encrypted,
	Size size,
	MXFMetadata const & metadata
	)
	: PictureAsset (directory, mxf_name, progress, fps, intrinsic_duration, encrypted, size)
{
	construct (boost::bind (&MonoPictureAsset::path_from_list, this, _1, files), metadata);
}

MonoPictureAsset::MonoPictureAsset (string directory, string mxf_name, int fps, Size size)
	: PictureAsset (directory, mxf_name, 0, fps, 0, false, size)
{

}

MonoPictureAsset::MonoPictureAsset (string directory, string mxf_name)
	: PictureAsset (directory, mxf_name)
{
	ASDCP::JP2K::MXFReader reader;
	if (ASDCP_FAILURE (reader.OpenRead (path().string().c_str()))) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", path().string()));
	}
	
	ASDCP::JP2K::PictureDescriptor desc;
	if (ASDCP_FAILURE (reader.FillPictureDescriptor (desc))) {
		boost::throw_exception (DCPReadError ("could not read video MXF information"));
	}

	_size.width = desc.StoredWidth;
	_size.height = desc.StoredHeight;
	_edit_rate = desc.EditRate.Numerator;
	assert (desc.EditRate.Denominator == 1);
	_intrinsic_duration = desc.ContainerDuration;
}

void
MonoPictureAsset::construct (boost::function<string (int)> get_path, MXFMetadata const & metadata)
{
	ASDCP::JP2K::CodestreamParser j2k_parser;
	ASDCP::JP2K::FrameBuffer frame_buffer (4 * Kumu::Megabyte);
	if (ASDCP_FAILURE (j2k_parser.OpenReadFrame (get_path(0).c_str(), frame_buffer))) {
		boost::throw_exception (FileError ("could not open JPEG2000 file for reading", get_path (0)));
	}
	
	ASDCP::JP2K::PictureDescriptor picture_desc;
	j2k_parser.FillPictureDescriptor (picture_desc);
	picture_desc.EditRate = ASDCP::Rational (_edit_rate, 1);
	
	ASDCP::WriterInfo writer_info;
	fill_writer_info (&writer_info, _uuid, metadata);
	
	ASDCP::JP2K::MXFWriter mxf_writer;
	if (ASDCP_FAILURE (mxf_writer.OpenWrite (path().string().c_str(), writer_info, picture_desc, 16384, false))) {
		boost::throw_exception (MXFFileError ("could not open MXF file for writing", path().string()));
	}

	for (int i = 0; i < _intrinsic_duration; ++i) {

		string const path = get_path (i);

		if (ASDCP_FAILURE (j2k_parser.OpenReadFrame (path.c_str(), frame_buffer))) {
			boost::throw_exception (FileError ("could not open JPEG2000 file for reading", path));
		}

		if (ASDCP_FAILURE (mxf_writer.WriteFrame (frame_buffer, _encryption_context, 0))) {
			boost::throw_exception (MXFFileError ("error in writing video MXF", this->path().string()));
		}

		if (_progress) {
			(*_progress) (0.5 * float (i) / _intrinsic_duration);
		}
	}
	
	if (ASDCP_FAILURE (mxf_writer.Finalize())) {
		boost::throw_exception (MXFFileError ("error in finalising video MXF", path().string()));
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
	return shared_ptr<const MonoPictureFrame> (new MonoPictureFrame (path().string(), n, _decryption_context));
}


bool
MonoPictureAsset::equals (shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (!PictureAsset::equals (other, opt, note)) {
		return false;
	}

	shared_ptr<const MonoPictureAsset> other_picture = dynamic_pointer_cast<const MonoPictureAsset> (other);
	assert (other_picture);

	for (int i = 0; i < _intrinsic_duration; ++i) {
		if (i >= other_picture->intrinsic_duration()) {
			return false;
		}
		
		note (PROGRESS, "Comparing video frame " + lexical_cast<string> (i) + " of " + lexical_cast<string> (_intrinsic_duration));
		shared_ptr<const MonoPictureFrame> frame_A = get_frame (i);
		shared_ptr<const MonoPictureFrame> frame_B = other_picture->get_frame (i);
		
		if (!frame_buffer_equals (
			    i, opt, note,
			    frame_A->j2k_data(), frame_A->j2k_size(),
			    frame_B->j2k_data(), frame_B->j2k_size()
			    )) {
			return false;
		}
	}

	return true;
}

bool
StereoPictureAsset::equals (shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (!PictureAsset::equals (other, opt, note)) {
		return false;
	}
	
	shared_ptr<const StereoPictureAsset> other_picture = dynamic_pointer_cast<const StereoPictureAsset> (other);
	assert (other_picture);

	for (int i = 0; i < _intrinsic_duration; ++i) {
		shared_ptr<const StereoPictureFrame> frame_A = get_frame (i);
		shared_ptr<const StereoPictureFrame> frame_B = other_picture->get_frame (i);
		
		if (!frame_buffer_equals (
			    i, opt, note,
			    frame_A->left_j2k_data(), frame_A->left_j2k_size(),
			    frame_B->left_j2k_data(), frame_B->left_j2k_size()
			    )) {
			return false;
		}
		
		if (!frame_buffer_equals (
			    i, opt, note,
			    frame_A->right_j2k_data(), frame_A->right_j2k_size(),
			    frame_B->right_j2k_data(), frame_B->right_j2k_size()
			    )) {
			return false;
		}
	}

	return true;
}

bool
PictureAsset::frame_buffer_equals (
	int frame, EqualityOptions opt, boost::function<void (NoteType, string)> note,
	uint8_t const * data_A, unsigned int size_A, uint8_t const * data_B, unsigned int size_B
	) const
{
	if (size_A == size_B && memcmp (data_A, data_B, size_A) == 0) {
		note (NOTE, "J2K identical");
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
			note (ERROR, "image sizes for frame " + lexical_cast<string>(frame) + " differ");
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
	
	note (NOTE, "mean difference " + lexical_cast<string> (mean) + ", deviation " + lexical_cast<string> (std_dev));
	
	if (mean > opt.max_mean_pixel_error) {
		note (ERROR, "mean " + lexical_cast<string>(mean) + " out of range " + lexical_cast<string>(opt.max_mean_pixel_error) + " in frame " + lexical_cast<string>(frame));
		return false;
	}

	if (std_dev > opt.max_std_dev_pixel_error) {
		note (ERROR, "standard deviation " + lexical_cast<string>(std_dev) + " out of range " + lexical_cast<string>(opt.max_std_dev_pixel_error) + " in frame " + lexical_cast<string>(frame));
		return false;
	}

	return true;
}


StereoPictureAsset::StereoPictureAsset (string directory, string mxf_name, int fps, int intrinsic_duration)
	: PictureAsset (directory, mxf_name, 0, fps, intrinsic_duration, false, Size (0, 0))
{
	ASDCP::JP2K::MXFSReader reader;
	if (ASDCP_FAILURE (reader.OpenRead (path().string().c_str()))) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", path().string()));
	}
	
	ASDCP::JP2K::PictureDescriptor desc;
	if (ASDCP_FAILURE (reader.FillPictureDescriptor (desc))) {
		boost::throw_exception (DCPReadError ("could not read video MXF information"));
	}

	_size.width = desc.StoredWidth;
	_size.height = desc.StoredHeight;
}

shared_ptr<const StereoPictureFrame>
StereoPictureAsset::get_frame (int n) const
{
	return shared_ptr<const StereoPictureFrame> (new StereoPictureFrame (path().string(), n));
}

shared_ptr<PictureAssetWriter>
MonoPictureAsset::start_write (bool overwrite, MXFMetadata const & metadata)
{
	/* XXX: can't we use shared_ptr here? */
	return shared_ptr<MonoPictureAssetWriter> (new MonoPictureAssetWriter (this, overwrite, metadata));
}

string
PictureAsset::key_type () const
{
	return "MDIK";
}

StereoPictureAsset::StereoPictureAsset (string directory, string mxf_name, int fps, Size size)
	: PictureAsset (directory, mxf_name, 0, fps, 0, false, size)
{

}

shared_ptr<PictureAssetWriter>
StereoPictureAsset::start_write (bool overwrite, MXFMetadata const & metadata)
{
	/* XXX: can't we use shared_ptr here? */
	return shared_ptr<StereoPictureAssetWriter> (new StereoPictureAssetWriter (this, overwrite, metadata));
}

