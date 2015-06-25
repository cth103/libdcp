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

/** Image must be packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, with the 2-byte value for each R/G/B component stored as little-endian;
 *  i.e. AV_PIX_FMT_RGB48LE.
 */
shared_ptr<libdcp::XYZFrame>
libdcp::rgb_to_xyz (
	shared_ptr<const Image> rgb, shared_ptr<const LUT> lut_in, shared_ptr<const LUT> lut_out,
	double const rgb_to_xyz[3][3], double const bradford[3][3]
	)
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

	struct {
		double x, y, z;
	} e;

	int jn = 0;
	for (int y = 0; y < rgb->size().height; ++y) {
		uint16_t* p = reinterpret_cast<uint16_t *> (rgb->data()[0] + y * rgb->stride()[0]);
		for (int x = 0; x < rgb->size().width; ++x) {

			/* In gamma LUT (truncating 16-bit to 12-bit) */
			s.r = lut_in->lut()[*p++ >> 4];
			s.g = lut_in->lut()[*p++ >> 4];
			s.b = lut_in->lut()[*p++ >> 4];

			/* RGB to XYZ Matrix */
			d.x = ((s.r * rgb_to_xyz[0][0]) +
			       (s.g * rgb_to_xyz[0][1]) +
			       (s.b * rgb_to_xyz[0][2]));

			d.y = ((s.r * rgb_to_xyz[1][0]) +
			       (s.g * rgb_to_xyz[1][1]) +
			       (s.b * rgb_to_xyz[1][2]));

			d.z = ((s.r * rgb_to_xyz[2][0]) +
			       (s.g * rgb_to_xyz[2][1]) +
			       (s.b * rgb_to_xyz[2][2]));

			/* Bradford matrix */
			e.x = ((d.x * bradford[0][0]) +
			       (d.y * bradford[0][1]) +
			       (d.z * bradford[0][2]));

			e.y = ((d.x * bradford[1][0]) +
			       (d.y * bradford[1][1]) +
			       (d.z * bradford[1][2]));

			e.z = ((d.x * bradford[2][0]) +
			       (d.y * bradford[2][1]) +
			       (d.z * bradford[2][2]));

			/* DCI companding */
			e.x = e.x * DCI_COEFFICIENT * 65535;
			e.y = e.y * DCI_COEFFICIENT * 65535;
			e.z = e.z * DCI_COEFFICIENT * 65535;

			assert (e.x >= 0 && e.x < 65536);
			assert (e.y >= 0 && e.y < 65536);
			assert (e.z >= 0 && e.z < 65536);

			/* Out gamma LUT */
			xyz->data(0)[jn] = lut_out->lut()[(int) e.x] * 4096;
			xyz->data(1)[jn] = lut_out->lut()[(int) e.y] * 4096;
			xyz->data(2)[jn] = lut_out->lut()[(int) e.z] * 4096;

			++jn;
		}
	}

	return xyz;
}

/** Image must be packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, with the 2-byte value for each R/G/B component stored as little-endian;
 *  i.e. AV_PIX_FMT_RGB48LE.
 */
shared_ptr<libdcp::XYZFrame>
libdcp::xyz_to_xyz (shared_ptr<const Image> xyz_16)
{
	shared_ptr<XYZFrame> xyz_12 (new XYZFrame (xyz_16->size ()));

	int jn = 0;
	for (int y = 0; y < xyz_16->size().height; ++y) {
		uint16_t* p = reinterpret_cast<uint16_t *> (xyz_16->data()[0] + y * xyz_16->stride()[0]);
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
