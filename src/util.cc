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

/** @file  src/util.cc
 *  @brief Utility methods.
 */

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <openssl/sha.h>
#include "KM_util.h"
#include "KM_fileio.h"
#include "AS_DCP.h"
#include "util.h"
#include "exceptions.h"
#include "types.h"
#include "argb_frame.h"
#include "lut.h"

using namespace std;
using namespace boost;
using namespace libdcp;

string
libdcp::make_uuid ()
{
	char buffer[64];
	Kumu::UUID id;
	Kumu::GenRandomValue (id);
	id.EncodeHex (buffer, 64);
	return string (buffer);
}

string
libdcp::make_digest (string filename, sigc::signal1<void, float>* progress)
{
	int const file_size = filesystem::file_size (filename);
	
	Kumu::FileReader reader;
	if (ASDCP_FAILURE (reader.OpenRead (filename.c_str ()))) {
		throw FileError ("could not open file to compute digest", filename);
	}
	
	SHA_CTX sha;
	SHA1_Init (&sha);
	
	Kumu::ByteString read_buffer (65536);
	int done = 0;
	while (1) {
		ui32_t read = 0;
		Kumu::Result_t r = reader.Read (read_buffer.Data(), read_buffer.Capacity(), &read);
		
		if (r == Kumu::RESULT_ENDOFFILE) {
			break;
		} else if (ASDCP_FAILURE (r)) {
			throw FileError ("could not read file to compute digest", filename);
		}
		
		SHA1_Update (&sha, read_buffer.Data(), read);
		done += read;

		if (progress) {
			(*progress) (0.5 + (0.5 * done / file_size));
		}
	}

	byte_t byte_buffer[20];
	SHA1_Final (byte_buffer, &sha);

	stringstream s;
	char digest[64];
	return Kumu::base64encode (byte_buffer, 20, digest, 64);
}

string
libdcp::content_kind_to_string (ContentKind kind)
{
	switch (kind) {
	case FEATURE:
		return "feature";
	case SHORT:
		return "short";
	case TRAILER:
		return "trailer";
	case TEST:
		return "test";
	case TRANSITIONAL:
		return "transitional";
	case RATING:
		return "rating";
	case TEASER:
		return "teaser";
	case POLICY:
		return "policy";
	case PUBLIC_SERVICE_ANNOUNCEMENT:
		return "psa";
	case ADVERTISEMENT:
		return "advertisement";
	}

	assert (false);
}

libdcp::ContentKind
libdcp::content_kind_from_string (string type)
{
	if (type == "feature") {
		return FEATURE;
	} else if (type == "short") {
		return SHORT;
	} else if (type == "trailer" || type == "Trailer") {
		return TRAILER;
	} else if (type == "test") {
		return TEST;
	} else if (type == "transitional") {
		return TRANSITIONAL;
	} else if (type == "rating") {
		return RATING;
	} else if (type == "teaser") {
		return TEASER;
	} else if (type == "policy") {
		return POLICY;
	} else if (type == "psa") {
		return PUBLIC_SERVICE_ANNOUNCEMENT;
	} else if (type == "advertisement") {
		return ADVERTISEMENT;
	}

	assert (false);
}
		
bool
libdcp::ends_with (string big, string little)
{
	if (little.size() > big.size()) {
		return false;
	}

	return big.compare (big.length() - little.length(), little.length(), little) == 0;
}

opj_image_t *
libdcp::decompress_j2k (uint8_t* data, int64_t size, int reduce)
{
	opj_dinfo_t* decoder = opj_create_decompress (CODEC_J2K);
	opj_dparameters_t parameters;
	opj_set_default_decoder_parameters (&parameters);
	parameters.cp_reduce = reduce;
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

shared_ptr<ARGBFrame>
libdcp::xyz_to_rgb (opj_image_t* xyz_frame)
{
	struct {
		double x, y, z;
	} s;
	
	struct {
		double r, g, b;
	} d;
	
	int* xyz_x = xyz_frame->comps[0].data;
	int* xyz_y = xyz_frame->comps[1].data;
	int* xyz_z = xyz_frame->comps[2].data;

	shared_ptr<ARGBFrame> argb_frame (new ARGBFrame (xyz_frame->x1, xyz_frame->y1));
	
	uint8_t* argb = argb_frame->data ();
	
	for (int y = 0; y < xyz_frame->y1; ++y) {
		uint8_t* argb_line = argb;
		for (int x = 0; x < xyz_frame->x1; ++x) {
			
			assert (*xyz_x >= 0 && *xyz_y >= 0 && *xyz_z >= 0 && *xyz_x < 4096 && *xyz_x < 4096 && *xyz_z < 4096);
			
			/* In gamma LUT */
			s.x = lut_in[*xyz_x++];
			s.y = lut_in[*xyz_y++];
			s.z = lut_in[*xyz_z++];
			
			/* DCI companding */
			s.x /= DCI_COEFFICIENT;
			s.y /= DCI_COEFFICIENT;
			s.z /= DCI_COEFFICIENT;
			
			/* XYZ to RGB */
			d.r = ((s.x * color_matrix[0][0]) + (s.y * color_matrix[0][1]) + (s.z * color_matrix[0][2]));
			d.g = ((s.x * color_matrix[1][0]) + (s.y * color_matrix[1][1]) + (s.z * color_matrix[1][2]));
			d.b = ((s.x * color_matrix[2][0]) + (s.y * color_matrix[2][1]) + (s.z * color_matrix[2][2]));
			
			d.r = min (d.r, 1.0);
			d.r = max (d.r, 0.0);
			
			d.g = min (d.g, 1.0);
			d.g = max (d.g, 0.0);
			
			d.b = min (d.b, 1.0);
			d.b = max (d.b, 0.0);
			
			/* Out gamma LUT */
			*argb_line++ = lut_out[(int) (d.b * COLOR_DEPTH)];
			*argb_line++ = lut_out[(int) (d.g * COLOR_DEPTH)];
			*argb_line++ = lut_out[(int) (d.r * COLOR_DEPTH)];
			*argb_line++ = 0xff;
		}
		
		argb += argb_frame->stride ();
	}

	return argb_frame;
}
