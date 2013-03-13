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
#include "gamma_lut.h"
#include "xyz_srgb_lut.h"

using std::string;
using std::stringstream;
using std::min;
using std::max;
using boost::shared_ptr;
using namespace libdcp;

/** Create a UUID.
 *  @return UUID.
 */
string
libdcp::make_uuid ()
{
	char buffer[64];
	Kumu::UUID id;
	Kumu::GenRandomValue (id);
	id.EncodeHex (buffer, 64);
	return string (buffer);
}


/** Create a digest for a file.
 *  @param filename File name.
 *  @return Digest.
 */
string
libdcp::make_digest (string filename)
{
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
	}

	byte_t byte_buffer[20];
	SHA1_Final (byte_buffer, &sha);

	stringstream s;
	char digest[64];
	return Kumu::base64encode (byte_buffer, 20, digest, 64);
}

/** Convert a content kind to a string which can be used in a
 *  <ContentKind> node.
 *  @param kind ContentKind.
 *  @return string.
 */
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

/** Convert a string from a <ContentKind> node to a libdcp ContentKind.
 *  Reasonably tolerant about varying case.
 *  @param type Content kind string.
 *  @return libdcp ContentKind.
 */
libdcp::ContentKind
libdcp::content_kind_from_string (string type)
{
	/* XXX: should probably just convert type to lower-case and have done with it */
	
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
	} else if (type == "teaser" || type == "Teaser") {
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

/** Decompress a JPEG2000 image to a bitmap.
 *  @param data JPEG2000 data.
 *  @param size Size of data in bytes.
 *  @param reduce A power of 2 by which to reduce the size of the decoded image;
 *  e.g. 0 reduces by (2^0 == 1), ie keeping the same size.
 *       1 reduces by (2^1 == 2), ie halving the size of the image.
 *  This is useful for scaling 4K DCP images down to 2K.
 *  @return openjpeg image, which the caller must call opj_image_destroy() on.
 */
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

	image->x1 = rint (float(image->x1) / pow (2, reduce));
	image->y1 = rint (float(image->y1) / pow (2, reduce));
	return image;
}

/** Convert an openjpeg XYZ image to RGB.
 *  @param xyz_frame Frame in XYZ.
 *  @return RGB image.
 */
shared_ptr<ARGBFrame>
libdcp::xyz_to_rgb (opj_image_t* xyz_frame, shared_ptr<const GammaLUT> lut_in, shared_ptr<const XYZsRGBLUT> lut_out)
{
	float const dci_coefficient = 48.0 / 52.37;

        /* sRGB color matrix for XYZ -> RGB */
	float const colour_matrix[3][3] = {
		{ 3.240454836, -1.537138850, -0.498531547},
		{-0.969266390,  1.876010929,  0.041556082},
		{ 0.055643420, -0.204025854,  1.057225162}
	};

	int const max_colour = pow (2, lut_out->bit_depth()) - 1;

	struct {
		double x, y, z;
	} s;
	
	struct {
		double r, g, b;
	} d;
	
	int* xyz_x = xyz_frame->comps[0].data;
	int* xyz_y = xyz_frame->comps[1].data;
	int* xyz_z = xyz_frame->comps[2].data;

	shared_ptr<ARGBFrame> argb_frame (new ARGBFrame (Size (xyz_frame->x1, xyz_frame->y1)));

	uint8_t* argb = argb_frame->data ();
	
	for (int y = 0; y < xyz_frame->y1; ++y) {
		uint8_t* argb_line = argb;
		for (int x = 0; x < xyz_frame->x1; ++x) {

			assert (*xyz_x >= 0 && *xyz_y >= 0 && *xyz_z >= 0 && *xyz_x < 4096 && *xyz_x < 4096 && *xyz_z < 4096);
			
			/* In gamma LUT */
			s.x = lut_in->lut()[*xyz_x++];
			s.y = lut_in->lut()[*xyz_y++];
			s.z = lut_in->lut()[*xyz_z++];
			
			/* DCI companding */
			s.x /= dci_coefficient;
			s.y /= dci_coefficient;
			s.z /= dci_coefficient;
			
			/* XYZ to RGB */
			d.r = ((s.x * colour_matrix[0][0]) + (s.y * colour_matrix[0][1]) + (s.z * colour_matrix[0][2]));
			d.g = ((s.x * colour_matrix[1][0]) + (s.y * colour_matrix[1][1]) + (s.z * colour_matrix[1][2]));
			d.b = ((s.x * colour_matrix[2][0]) + (s.y * colour_matrix[2][1]) + (s.z * colour_matrix[2][2]));
			
			d.r = min (d.r, 1.0);
			d.r = max (d.r, 0.0);
			
			d.g = min (d.g, 1.0);
			d.g = max (d.g, 0.0);
			
			d.b = min (d.b, 1.0);
			d.b = max (d.b, 0.0);
			
			/* Out gamma LUT */
			*argb_line++ = lut_out->lut()[(int) (d.b * max_colour)];
			*argb_line++ = lut_out->lut()[(int) (d.g * max_colour)];
			*argb_line++ = lut_out->lut()[(int) (d.r * max_colour)];
			*argb_line++ = 0xff;
		}
		
		argb += argb_frame->stride ();
	}

	return argb_frame;
}

/** @param s A string.
 *  @return true if the string contains only space, newline or tab characters, or is empty.
 */
bool
libdcp::empty_or_white_space (string s)
{
	for (size_t i = 0; i < s.length(); ++i) {
		if (s[i] != ' ' && s[i] != '\n' && s[i] != '\t') {
			return false;
		}
	}

	return true;
}

bool libdcp::operator== (libdcp::Size const & a, libdcp::Size const & b)
{
	return (a.width == b.width && a.height == b.height);
}

bool libdcp::operator!= (libdcp::Size const & a, libdcp::Size const & b)
{
	return !(a == b);
}

