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
#include "openjpeg_image.h"
#include "colour_matrix.h"
#include "colour_conversion.h"
#include "transfer_function.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include <cmath>

using std::min;
using std::max;
using std::cout;
using boost::shared_ptr;
using boost::optional;
using namespace dcp;

#define DCI_COEFFICIENT (48.0 / 52.37)

/** Convert an XYZ image to RGBA.
 *  @param xyz_image Image in XYZ.
 *  @param conversion Colour conversion to use.
 *  @param argb Buffer to fill with RGBA data.  The format of the data is:
 *
 *  <pre>
 *  Byte   /- 0 -------|- 1 --------|- 2 --------|- 3 --------|- 4 --------|- 5 --------| ...
 *         |(0, 0) Blue|(0, 0)Green |(0, 0) Red  |(0, 0) Alpha|(0, 1) Blue |(0, 1) Green| ...
 *  </pre>
 *
 *  So that the first byte is the blue component of the pixel at x=0, y=0, the second
 *  is the green component, and so on.
 *
 *  Lines are packed so that the second row directly follows the first.
 */
void
dcp::xyz_to_rgba (
	boost::shared_ptr<const OpenJPEGImage> xyz_image,
	ColourConversion const & conversion,
	uint8_t* argb
	)
{
	int const max_colour = pow (2, 12) - 1;

	struct {
		double x, y, z;
	} s;

	struct {
		double r, g, b;
	} d;

	int* xyz_x = xyz_image->data (0);
	int* xyz_y = xyz_image->data (1);
	int* xyz_z = xyz_image->data (2);

	double const * lut_in = conversion.out()->lut (12, false);
	double const * lut_out = conversion.in()->lut (16, true);
	boost::numeric::ublas::matrix<double> const matrix = conversion.xyz_to_rgb ();

	int const height = xyz_image->size().height;
	int const width = xyz_image->size().width;

	for (int y = 0; y < height; ++y) {
		uint8_t* argb_line = argb;
		for (int x = 0; x < width; ++x) {

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

		/* 4 bytes per pixel */
		argb += width * 4;
	}
}

/** Convert an XYZ image to 48bpp RGB.
 *  @param xyz_image Frame in XYZ.
 *  @param conversion Colour conversion to use.
 *  @param rgb Buffer to fill with RGB data.  Format is packed RGB
 *  16:16:16, 48bpp, 16R, 16G, 16B, with the 2-byte value for each
 *  R/G/B component stored as little-endian; i.e. AV_PIX_FMT_RGB48LE.
 *  @param stride Stride for RGB data in bytes.
 *  @param note Optional handler for any notes that may be made during the conversion (e.g. when clamping occurs).
 */
void
dcp::xyz_to_rgb (
	shared_ptr<const OpenJPEGImage> xyz_image,
	ColourConversion const & conversion,
	uint8_t* rgb,
	int stride,
	optional<NoteHandler> note
	)
{
	struct {
		double x, y, z;
	} s;

	struct {
		double r, g, b;
	} d;

	/* These should be 12-bit values from 0-4095 */
	int* xyz_x = xyz_image->data (0);
	int* xyz_y = xyz_image->data (1);
	int* xyz_z = xyz_image->data (2);

	double const * lut_in = conversion.out()->lut (12, false);
	double const * lut_out = conversion.in()->lut (16, true);
	boost::numeric::ublas::matrix<double> const matrix = conversion.xyz_to_rgb ();

	for (int y = 0; y < xyz_image->size().height; ++y) {
		uint16_t* rgb_line = reinterpret_cast<uint16_t*> (rgb + y * stride);
		for (int x = 0; x < xyz_image->size().width; ++x) {

			int cx = *xyz_x++;
			int cy = *xyz_y++;
			int cz = *xyz_z++;

			if (cx < 0 || cx > 4095) {
				if (note) {
					note.get() (DCP_NOTE, String::compose ("XYZ value %1 out of range", cx));
				}
				cx = max (min (cx, 4095), 0);
			}

			if (cy < 0 || cy > 4095) {
				if (note) {
					note.get() (DCP_NOTE, String::compose ("XYZ value %1 out of range", cy));
				}
				cy = max (min (cy, 4095), 0);
			}

			if (cz < 0 || cz > 4095) {
				if (note) {
					note.get() (DCP_NOTE, String::compose ("XYZ value %1 out of range", cz));
				}
				cz = max (min (cz, 4095), 0);
			}

			/* In gamma LUT */
			s.x = lut_in[cx];
			s.y = lut_in[cy];
			s.z = lut_in[cz];

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

			*rgb_line++ = rint(lut_out[int(rint(d.r * 65535))] * 65535);
			*rgb_line++ = rint(lut_out[int(rint(d.g * 65535))] * 65535);
			*rgb_line++ = rint(lut_out[int(rint(d.b * 65535))] * 65535);
		}
	}
}

/** @param rgb RGB data; packed RGB 16:16:16, 48bpp, 16R, 16G, 16B,
 *  with the 2-byte value for each R/G/B component stored as
 *  little-endian; i.e. AV_PIX_FMT_RGB48LE.
 *  @param size of RGB image in pixels.
 *  @param stride of RGB data in pixels.
 */
shared_ptr<dcp::OpenJPEGImage>
dcp::rgb_to_xyz (
	uint8_t const * rgb,
	dcp::Size size,
	int stride,
	ColourConversion const & conversion,
	optional<NoteHandler> note
	)
{
	shared_ptr<OpenJPEGImage> xyz (new OpenJPEGImage (size));

	struct {
		double r, g, b;
	} s;

	struct {
		double x, y, z;
	} d;

	struct {
		double x, y, z;
	} e;

	double const * lut_in = conversion.in()->lut (12, false);
	double const * lut_out = conversion.out()->lut (16, true);
	boost::numeric::ublas::matrix<double> const rgb_to_xyz = conversion.rgb_to_xyz ();
	boost::numeric::ublas::matrix<double> const bradford = conversion.bradford ();

	int clamped = 0;
	int jn = 0;
	for (int y = 0; y < size.height; ++y) {
		uint16_t const * p = reinterpret_cast<uint16_t const *> (rgb + y * stride);
		for (int x = 0; x < size.width; ++x) {

			/* In gamma LUT (converting 16-bit to 12-bit) */
			s.r = lut_in[*p++ >> 4];
			s.g = lut_in[*p++ >> 4];
			s.b = lut_in[*p++ >> 4];

			/* RGB to XYZ Matrix */
			d.x = ((s.r * rgb_to_xyz(0, 0)) + (s.g * rgb_to_xyz(0, 1)) + (s.b * rgb_to_xyz(0, 2)));
			d.y = ((s.r * rgb_to_xyz(1, 0)) + (s.g * rgb_to_xyz(1, 1)) + (s.b * rgb_to_xyz(1, 2)));
			d.z = ((s.r * rgb_to_xyz(2, 0)) + (s.g * rgb_to_xyz(2, 1)) + (s.b * rgb_to_xyz(2, 2)));

			e.x = ((d.x * bradford(0, 0)) + (d.y * bradford(0, 1)) + (d.z * bradford(0, 2)));
			e.y = ((d.x * bradford(1, 0)) + (d.y * bradford(1, 1)) + (d.z * bradford(1, 2)));
			e.z = ((d.x * bradford(2, 0)) + (d.y * bradford(2, 1)) + (d.z * bradford(2, 2)));

			/* DCI companding */
			e.x = e.x * DCI_COEFFICIENT * 65535;
			e.y = e.y * DCI_COEFFICIENT * 65535;
			e.z = e.z * DCI_COEFFICIENT * 65535;

			/* Clamp */

			if (e.x < 0 || e.y < 0 || e.z < 0 || e.x > 65535 || e.y > 65535 || e.z > 65535) {
				++clamped;
			}

			e.x = max (0.0, e.x);
			e.y = max (0.0, e.y);
			e.z = max (0.0, e.z);
			e.x = min (65535.0, e.x);
			e.y = min (65535.0, e.y);
			e.z = min (65535.0, e.z);

			/* Out gamma LUT */
			xyz->data(0)[jn] = lut_out[int(rint(e.x))] * 4095;
			xyz->data(1)[jn] = lut_out[int(rint(e.y))] * 4095;
			xyz->data(2)[jn] = lut_out[int(rint(e.z))] * 4095;

			++jn;
		}
	}

	if (clamped && note) {
		note.get() (DCP_NOTE, String::compose ("%1 XYZ value(s) clamped", clamped));
	}

	return xyz;
}


/** @param xyz_16 XYZ image data in packed 16:16:16, 48bpp, 16X, 16Y,
 *  16Z, with the 2-byte value for each X/Y/Z component stored as
 *  little-endian.
 */
shared_ptr<dcp::OpenJPEGImage>
dcp::xyz_to_xyz (uint8_t const * xyz_16, dcp::Size size, int stride)
{
	shared_ptr<OpenJPEGImage> xyz_12 (new OpenJPEGImage (size));

	int jn = 0;
	for (int y = 0; y < size.height; ++y) {
		uint16_t const * p = reinterpret_cast<uint16_t const *> (xyz_16 + y * stride);
		for (int x = 0; x < size.width; ++x) {
			/* Truncate 16-bit to 12-bit */
			xyz_12->data(0)[jn] = *p++ >> 4;
			xyz_12->data(1)[jn] = *p++ >> 4;
			xyz_12->data(2)[jn] = *p++ >> 4;
			++jn;
		}
	}

	return xyz_12;
}
