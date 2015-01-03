/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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

#include "rgb_xyz.h"
#include "argb_frame.h"
#include "xyz_frame.h"
#include "image.h"
#include "colour_matrix.h"
#include "colour_conversion.h"
#include "transfer_function.h"
#include "dcp_assert.h"
#include <cmath>

using std::min;
using std::max;
using boost::shared_ptr;
using namespace dcp;

#define DCI_COEFFICIENT (48.0 / 52.37)

/** Convert an openjpeg XYZ image to RGBA.
 *  @param xyz_frame Frame in XYZ.
 *  @return RGB image.
 */
shared_ptr<ARGBFrame>
dcp::xyz_to_rgba (
	boost::shared_ptr<const XYZFrame> xyz_frame,
	ColourConversion const & conversion
	)
{
	int const max_colour = pow (2, 12) - 1;

	struct {
		double x, y, z;
	} s;
	
	struct {
		double r, g, b;
	} d;
	
	int* xyz_x = xyz_frame->data (0);
	int* xyz_y = xyz_frame->data (1);
	int* xyz_z = xyz_frame->data (2);

	shared_ptr<ARGBFrame> argb_frame (new ARGBFrame (xyz_frame->size ()));
	uint8_t* argb = argb_frame->data ();
	
	double const * lut_in = conversion.in()->lut (16);
	double const * lut_out = conversion.out()->lut (12);
	boost::numeric::ublas::matrix<double> matrix = conversion.matrix ();

	for (int y = 0; y < xyz_frame->size().height; ++y) {
		uint8_t* argb_line = argb;
		for (int x = 0; x < xyz_frame->size().width; ++x) {

			DCP_ASSERT (*xyz_x >= 0 && *xyz_y >= 0 && *xyz_z >= 0 && *xyz_x < 4096 && *xyz_y < 4096 && *xyz_z < 4096);
			
			/* In gamma LUT */
			s.x = lut_in[*xyz_x++];
			s.y = lut_in[*xyz_y++];
			s.z = lut_in[*xyz_z++];

			/* DCI companding */
			s.x /= DCI_COEFFICIENT;
			s.y /= DCI_COEFFICIENT;
			s.z /= DCI_COEFFICIENT;

			/* XYZ to RGB */
			d.r = ((s.x * matrix(0, 0)) + (s.y * matrix(0, 1)) + (s.z * matrix(0, 2)));
			d.g = ((s.x * matrix(1, 0)) + (s.y * matrix(1, 1)) + (s.z * matrix(1, 2)));
			d.b = ((s.x * matrix(2, 0)) + (s.y * matrix(2, 1)) + (s.z * matrix(2, 2)));
			
			d.r = min (d.r, 1.0);
			d.r = max (d.r, 0.0);
			
			d.g = min (d.g, 1.0);
			d.g = max (d.g, 0.0);
			
			d.b = min (d.b, 1.0);
			d.b = max (d.b, 0.0);
			
			/* Out gamma LUT */
			*argb_line++ = lut_out[int(rint(d.b * max_colour))] * 0xff;
			*argb_line++ = lut_out[int(rint(d.g * max_colour))] * 0xff;
			*argb_line++ = lut_out[int(rint(d.r * max_colour))] * 0xff;
			*argb_line++ = 0xff;
		}
		
		argb += argb_frame->stride ();
	}

	return argb_frame;
}

/** Convert an openjpeg XYZ image to RGB.
 *  @param xyz_frame Frame in XYZ.
 *  @param conversion Colour conversion to use.
 *  @param buffer Buffer to write RGB data to; rgb will be packed RGB
 *  16:16:16, 48bpp, 16R, 16G, 16B, with the 2-byte value for each
 *  R/G/B component stored as little-endian; i.e. AV_PIX_FMT_RGB48LE.
 */
void
dcp::xyz_to_rgb (
	boost::shared_ptr<const XYZFrame> xyz_frame,
	ColourConversion const & conversion,
	uint16_t* buffer
	)
{
	struct {
		double x, y, z;
	} s;
	
	struct {
		double r, g, b;
	} d;

	/* These should be 12-bit values from 0-4095 */
	int* xyz_x = xyz_frame->data (0);
	int* xyz_y = xyz_frame->data (1);
	int* xyz_z = xyz_frame->data (2);

	double const * lut_in = conversion.in()->lut (12);
	double const * lut_out = conversion.out()->lut (16);
	boost::numeric::ublas::matrix<double> matrix = conversion.matrix ();
	
	for (int y = 0; y < xyz_frame->size().height; ++y) {
		uint16_t* buffer_line = buffer;
		for (int x = 0; x < xyz_frame->size().width; ++x) {

			DCP_ASSERT (*xyz_x >= 0 && *xyz_y >= 0 && *xyz_z >= 0 && *xyz_x < 4096 && *xyz_y < 4096 && *xyz_z < 4096);

			/* In gamma LUT */
			s.x = lut_in[*xyz_x++];
			s.y = lut_in[*xyz_y++];
			s.z = lut_in[*xyz_z++];

			/* DCI companding */
			s.x /= DCI_COEFFICIENT;
			s.y /= DCI_COEFFICIENT;
			s.z /= DCI_COEFFICIENT;

			/* XYZ to RGB */
			d.r = ((s.x * matrix(0, 0)) + (s.y * matrix(0, 1)) + (s.z * matrix(0, 2)));
			d.g = ((s.x * matrix(1, 0)) + (s.y * matrix(1, 1)) + (s.z * matrix(1, 2)));
			d.b = ((s.x * matrix(2, 0)) + (s.y * matrix(2, 1)) + (s.z * matrix(2, 2)));
			
			d.r = min (d.r, 1.0);
			d.r = max (d.r, 0.0);
			
			d.g = min (d.g, 1.0);
			d.g = max (d.g, 0.0);
			
			d.b = min (d.b, 1.0);
			d.b = max (d.b, 0.0);

			*buffer_line++ = rint(lut_out[int(rint(d.r * 65535))] * 65535);
			*buffer_line++ = rint(lut_out[int(rint(d.g * 65535))] * 65535);
			*buffer_line++ = rint(lut_out[int(rint(d.b * 65535))] * 65535);
		}
		
		buffer += xyz_frame->size().width * 3;
	}
}

/** rgb must be packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, with the 2-byte value for each R/G/B component stored as little-endian;
 *  i.e. AV_PIX_FMT_RGB48LE.
 */
shared_ptr<dcp::XYZFrame>
dcp::rgb_to_xyz (
	boost::shared_ptr<const Image> rgb,
	ColourConversion const & conversion
	)
{
	shared_ptr<XYZFrame> xyz (new XYZFrame (rgb->size ()));

	struct {
		double r, g, b;
	} s;

	struct {
		double x, y, z;
	} d;

	double const * lut_in = conversion.in()->lut (12);
	double const * lut_out = conversion.out()->lut (16);
	boost::numeric::ublas::matrix<double> matrix = conversion.matrix ();

	int jn = 0;
	for (int y = 0; y < rgb->size().height; ++y) {
		uint16_t* p = reinterpret_cast<uint16_t*> (rgb->data()[0] + y * rgb->stride()[0]);
		for (int x = 0; x < rgb->size().width; ++x) {

			/* In gamma LUT (converting 16-bit to 12-bit) */
			s.r = lut_in[*p++ >> 4];
			s.g = lut_in[*p++ >> 4];
			s.b = lut_in[*p++ >> 4];

			/* RGB to XYZ Matrix */
			d.x = ((s.r * matrix(0, 0)) + (s.g * matrix(0, 1)) + (s.b * matrix(0, 2)));
			d.y = ((s.r * matrix(1, 0)) + (s.g * matrix(1, 1)) + (s.b * matrix(1, 2)));
			d.z = ((s.r * matrix(2, 0)) + (s.g * matrix(2, 1)) + (s.b * matrix(2, 2)));
			
			/* DCI companding */
			d.x = d.x * DCI_COEFFICIENT * 65535;
			d.y = d.y * DCI_COEFFICIENT * 65535;
			d.z = d.z * DCI_COEFFICIENT * 65535;

			DCP_ASSERT (d.x >= 0 && d.x < 65536);
			DCP_ASSERT (d.y >= 0 && d.y < 65536);
			DCP_ASSERT (d.z >= 0 && d.z < 65536);
			
			/* Out gamma LUT */
			xyz->data(0)[jn] = lut_out[int(rint(d.x))] * 4095;
			xyz->data(1)[jn] = lut_out[int(rint(d.y))] * 4095;
			xyz->data(2)[jn] = lut_out[int(rint(d.z))] * 4095;

			++jn;
		}
	}

	return xyz;
}


/** Image must be packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, with the 2-byte value for each R/G/B component stored as little-endian;
 *  i.e. AV_PIX_FMT_RGB48LE.
 */
shared_ptr<dcp::XYZFrame>
dcp::xyz_to_xyz (shared_ptr<const Image> xyz_16)
{
	shared_ptr<XYZFrame> xyz_12 (new XYZFrame (xyz_16->size ()));

	int jn = 0;
	for (int y = 0; y < xyz_16->size().height; ++y) {
		uint16_t* p = reinterpret_cast<uint16_t*> (xyz_16->data()[0] + y * xyz_16->stride()[0]);
		for (int x = 0; x < xyz_16->size().width; ++x) {
			/* Truncate 16-bit to 12-bit */
			xyz_12->data(0)[jn] = *p++ >> 4;
			xyz_12->data(1)[jn] = *p++ >> 4;
			xyz_12->data(2)[jn] = *p++ >> 4;
			++jn;
		}
	}
	
	return xyz_12;
}
