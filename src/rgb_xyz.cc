/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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
#include "gamma_lut.h"
#include "image.h"
#include "colour_matrix.h"

using std::min;
using std::max;
using boost::shared_ptr;
using namespace libdcp;

#define DCI_COEFFICIENT (48.0 / 52.37)

/** Convert an openjpeg XYZ image to RGB.
 *  @param xyz_frame Frame in XYZ.
 *  @return RGB image.
 */
shared_ptr<ARGBFrame>
libdcp::xyz_to_rgb (shared_ptr<const XYZFrame> xyz_frame, shared_ptr<const LUT> lut_in, shared_ptr<const LUT> lut_out)
{
	int const max_colour = pow (2, lut_out->bit_depth()) - 1;

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
	
	for (int y = 0; y < xyz_frame->size().height; ++y) {
		uint8_t* argb_line = argb;
		for (int x = 0; x < xyz_frame->size().width; ++x) {

			assert (*xyz_x >= 0 && *xyz_y >= 0 && *xyz_z >= 0 && *xyz_x < 4096 && *xyz_y < 4096 && *xyz_z < 4096);
			
			/* In gamma LUT */
			s.x = lut_in->lut()[*xyz_x++];
			s.y = lut_in->lut()[*xyz_y++];
			s.z = lut_in->lut()[*xyz_z++];

			/* DCI companding */
			s.x /= DCI_COEFFICIENT;
			s.y /= DCI_COEFFICIENT;
			s.z /= DCI_COEFFICIENT;

			/* XYZ to RGB */
			d.r = ((s.x * colour_matrix::xyz_to_rgb[0][0]) + (s.y * colour_matrix::xyz_to_rgb[0][1]) + (s.z * colour_matrix::xyz_to_rgb[0][2]));
			d.g = ((s.x * colour_matrix::xyz_to_rgb[1][0]) + (s.y * colour_matrix::xyz_to_rgb[1][1]) + (s.z * colour_matrix::xyz_to_rgb[1][2]));
			d.b = ((s.x * colour_matrix::xyz_to_rgb[2][0]) + (s.y * colour_matrix::xyz_to_rgb[2][1]) + (s.z * colour_matrix::xyz_to_rgb[2][2]));
			
			d.r = min (d.r, 1.0);
			d.r = max (d.r, 0.0);
			
			d.g = min (d.g, 1.0);
			d.g = max (d.g, 0.0);
			
			d.b = min (d.b, 1.0);
			d.b = max (d.b, 0.0);
			
			/* Out gamma LUT */
			*argb_line++ = lut_out->lut()[(int) (d.b * max_colour)] * 0xff;
			*argb_line++ = lut_out->lut()[(int) (d.g * max_colour)] * 0xff;
			*argb_line++ = lut_out->lut()[(int) (d.r * max_colour)] * 0xff;
			*argb_line++ = 0xff;
		}
		
		argb += argb_frame->stride ();
	}

	return argb_frame;
}

shared_ptr<libdcp::XYZFrame>
libdcp::rgb_to_xyz (shared_ptr<const Image> rgb, shared_ptr<const LUT> lut_in, shared_ptr<const LUT> lut_out, double const colour_matrix[3][3])
{
	assert (lut_in->bit_depth() == 12);
	assert (lut_out->bit_depth() == 16);
	
	shared_ptr<XYZFrame> xyz (new XYZFrame (rgb->size ()));

	struct {
		double r, g, b;
	} s;

	struct {
		double x, y, z;
	} d;

	int jn = 0;
	for (int y = 0; y < rgb->size().height; ++y) {
		uint8_t* p = rgb->data()[0] + y * rgb->stride()[0];
		for (int x = 0; x < rgb->size().width; ++x) {

			/* In gamma LUT (converting 8-bit input to 12-bit) */
			s.r = lut_in->lut()[*p++ << 4];
			s.g = lut_in->lut()[*p++ << 4];
			s.b = lut_in->lut()[*p++ << 4];
			
			/* RGB to XYZ Matrix */
			d.x = ((s.r * colour_matrix[0][0]) +
			       (s.g * colour_matrix[0][1]) +
			       (s.b * colour_matrix[0][2]));
			
			d.y = ((s.r * colour_matrix[1][0]) +
			       (s.g * colour_matrix[1][1]) +
			       (s.b * colour_matrix[1][2]));
			
			d.z = ((s.r * colour_matrix[2][0]) +
			       (s.g * colour_matrix[2][1]) +
			       (s.b * colour_matrix[2][2]));
			
			/* DCI companding */
			d.x = d.x * DCI_COEFFICIENT * 65535;
			d.y = d.y * DCI_COEFFICIENT * 65535;
			d.z = d.z * DCI_COEFFICIENT * 65535;

			assert (d.x >= 0 && d.x < 65536);
			assert (d.y >= 0 && d.y < 65536);
			assert (d.z >= 0 && d.z < 65536);
			
			/* Out gamma LUT */
			xyz->data(0)[jn] = lut_out->lut()[(int) d.x] * 4096;
			xyz->data(1)[jn] = lut_out->lut()[(int) d.y] * 4096;
			xyz->data(2)[jn] = lut_out->lut()[(int) d.z] * 4096;

			++jn;
		}
	}

	return xyz;
}
