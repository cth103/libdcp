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

using namespace std;
using namespace boost;
using namespace libdcp;

PictureAsset::PictureAsset (
	sigc::slot<string, int> get_path,
	string directory,
	string mxf_name,
	sigc::signal1<void, float>* progress,
	int fps,
	int length,
	int width,
	int height)
	: Asset (directory, mxf_name, progress, fps, length)
	, _width (width)
	, _height (height)
{
	construct (get_path);
}

PictureAsset::PictureAsset (
	vector<string> const & files,
	string directory,
	string mxf_name,
	sigc::signal1<void, float>* progress,
	int fps,
	int length,
	int width,
	int height)
	: Asset (directory, mxf_name, progress, fps, length)
	, _width (width)
	, _height (height)
{
	construct (sigc::bind (sigc::mem_fun (*this, &PictureAsset::path_from_list), files));
}

PictureAsset::PictureAsset (string directory, string mxf_name, int fps, int length, int width, int height)
	: Asset (directory, mxf_name, 0, fps, length)
	, _width (width)
	, _height (height)
{

}

string
PictureAsset::path_from_list (int f, vector<string> const & files) const
{
	return files[f];
}

void
PictureAsset::construct (sigc::slot<string, int> get_path)
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
	if (ASDCP_FAILURE (mxf_writer.OpenWrite (mxf_path().string().c_str(), writer_info, picture_desc))) {
		throw FileError ("could not open MXF file for writing", mxf_path().string());
	}

	for (int i = 0; i < _length; ++i) {

		string const path = get_path (i);
		
		if (ASDCP_FAILURE (j2k_parser.OpenReadFrame (path.c_str(), frame_buffer))) {
			throw FileError ("could not open JPEG2000 file for reading", path);
		}

		/* XXX: passing 0 to WriteFrame ok? */
		if (ASDCP_FAILURE (mxf_writer.WriteFrame (frame_buffer, 0, 0))) {
			throw MiscError ("error in writing video MXF");
		}
		
		(*_progress) (0.5 * float (i) / _length);
	}
	
	if (ASDCP_FAILURE (mxf_writer.Finalize())) {
		throw MiscError ("error in finalising video MXF");
	}
}

void
PictureAsset::write_to_cpl (ostream& s) const
{
	s << "        <MainPicture>\n"
	  << "          <Id>urn:uuid:" << _uuid << "</Id>\n"
	  << "          <AnnotationText>" << _mxf_name << "</AnnotationText>\n"
	  << "          <EditRate>" << _fps << " 1</EditRate>\n"
	  << "          <IntrinsicDuration>" << _length << "</IntrinsicDuration>\n"
	  << "          <EntryPoint>0</EntryPoint>\n"
	  << "          <Duration>" << _length << "</Duration>\n"
	  << "          <FrameRate>" << _fps << " 1</FrameRate>\n"
	  << "          <ScreenAspectRatio>" << _width << " " << _height << "</ScreenAspectRatio>\n"
	  << "        </MainPicture>\n";
}

list<string>
PictureAsset::equals (shared_ptr<const Asset> other, EqualityFlags flags, double max_mean, double max_std_dev) const
{
	list<string> notes = Asset::equals (other, flags, max_mean, max_std_dev);
		     
	if (flags & MXF_INSPECT) {
		ASDCP::JP2K::MXFReader reader_A;
		if (ASDCP_FAILURE (reader_A.OpenRead (mxf_path().string().c_str()))) {
			throw FileError ("could not open MXF file for reading", mxf_path().string());
		}

		ASDCP::JP2K::MXFReader reader_B;
		if (ASDCP_FAILURE (reader_B.OpenRead (other->mxf_path().string().c_str()))) {
			throw FileError ("could not open MXF file for reading", mxf_path().string());
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
//			desc_A.CodingStyleDefault != desc_B.CodingStyleDefault ||
//			desc_A.QuantizationDefault != desc_B.QuantizationDefault
			) {
		
			notes.push_back ("video MXF picture descriptors differ");
		}

//		for (unsigned int j = 0; j < ASDCP::JP2K::MaxComponents; ++j) {
//			if (desc_A.ImageComponents[j] != desc_B.ImageComponents[j]) {
//				notes.pack_start ("video MXF picture descriptors differ");
//			}
//		}
				

		ASDCP::JP2K::FrameBuffer buffer_A (4 * Kumu::Megabyte);
		ASDCP::JP2K::FrameBuffer buffer_B (4 * Kumu::Megabyte);

		for (int i = 0; i < _length; ++i) {
			if (ASDCP_FAILURE (reader_A.ReadFrame (i, buffer_A))) {
				throw DCPReadError ("could not read video frame");
			}

			if (ASDCP_FAILURE (reader_B.ReadFrame (i, buffer_B))) {
				throw DCPReadError ("could not read video frame");
			}

			bool j2k_same = true;

			if (buffer_A.Size() != buffer_B.Size()) {
				notes.push_back ("sizes of video data for frame " + lexical_cast<string>(i) + " differ");
				j2k_same = false;
			} else if (memcmp (buffer_A.RoData(), buffer_B.RoData(), buffer_A.Size()) != 0) {
				notes.push_back ("J2K data for frame " + lexical_cast<string>(i) + " differ");
				j2k_same = false;
			}

			if (!j2k_same) {
				/* Decompress the images to bitmaps */
				opj_image_t* image_A = decompress_j2k (const_cast<uint8_t*> (buffer_A.RoData()), buffer_A.Size ());
				opj_image_t* image_B = decompress_j2k (const_cast<uint8_t*> (buffer_B.RoData()), buffer_B.Size ());

				/* Compare them */
				
				if (image_A->numcomps != image_B->numcomps) {
					notes.push_back ("image component counts for frame " + lexical_cast<string>(i) + " differ");
				}

				vector<int> abs_diffs (image_A->comps[0].w * image_A->comps[0].h * image_A->numcomps);
				int d = 0;

				for (int c = 0; c < image_A->numcomps; ++c) {

					if (image_A->comps[c].w != image_B->comps[c].w || image_A->comps[c].h != image_B->comps[c].h) {
						notes.push_back ("image sizes for frame " + lexical_cast<string>(i) + " differ");
					}

					int const pixels = image_A->comps[c].w * image_A->comps[c].h;
					for (int j = 0; j < pixels; ++j) {
						abs_diffs[d++] = abs (image_A->comps[c].data[j] - image_B->comps[c].data[j]);
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

				if (mean > max_mean || std_dev > max_std_dev) {
					notes.push_back ("mean or standard deviation out of range for " + lexical_cast<string>(i));
				}

				opj_image_destroy (image_A);
				opj_image_destroy (image_B);
			}
		}
	}

	return notes;
}

opj_image_t *
PictureAsset::decompress_j2k (uint8_t* data, int64_t size) const
{
	opj_dinfo_t* decoder = opj_create_decompress (CODEC_J2K);
	opj_dparameters_t parameters;
	opj_set_default_decoder_parameters (&parameters);
	opj_setup_decoder (decoder, &parameters);
	opj_cio_t* cio = opj_cio_open ((opj_common_ptr) decoder, data, size);
	opj_image_t* image = opj_decode (decoder, cio);
	if (!image) {
		opj_destroy_decompress (decoder);
		opj_cio_close (cio);
		throw DCPReadError ("could not decode JPEG2000 codestream");
	}

	opj_cio_close (cio);
	return image;
}
