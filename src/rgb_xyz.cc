/*
    Copyright (C) 2013-2021 Carl Hetherington <cth@carlh.net>

    This file is part of libdcp.

    libdcp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libdcp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdcp.  If not, see <http://www.gnu.org/licenses/>.

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/


/** @file  rgb_xyz.cc
 *  @brief Conversion between RGB and XYZ
 */


#include "colour_conversion.h"
#include "compose.hpp"
#include "dcp_assert.h"
#include "openjpeg_image.h"
#include "rgb_xyz.h"
#include "transfer_function.h"
#include <cmath>


using std::cout;
using std::make_shared;
using std::max;
using std::min;
using std::shared_ptr;
using boost::optional;
using namespace dcp;


static auto constexpr DCI_COEFFICIENT = 48.0 / 52.37;


void
dcp::xyz_to_rgba (
	std::shared_ptr<const OpenJPEGImage> xyz_image,
	ColourConversion const & conversion,
	uint8_t* argb,
	int stride
	)
{
	int const max_colour = pow (2, 16) - 1;

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

	double fast_matrix[9] = {
		matrix (0, 0), matrix (0, 1), matrix (0, 2),
		matrix (1, 0), matrix (1, 1), matrix (1, 2),
		matrix (2, 0), matrix (2, 1), matrix (2, 2)
	};

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
			d.r = ((s.x * fast_matrix[0]) + (s.y * fast_matrix[1]) + (s.z * fast_matrix[2]));
			d.g = ((s.x * fast_matrix[3]) + (s.y * fast_matrix[4]) + (s.z * fast_matrix[5]));
			d.b = ((s.x * fast_matrix[6]) + (s.y * fast_matrix[7]) + (s.z * fast_matrix[8]));

			d.r = min (d.r, 1.0);
			d.r = max (d.r, 0.0);

			d.g = min (d.g, 1.0);
			d.g = max (d.g, 0.0);

			d.b = min (d.b, 1.0);
			d.b = max (d.b, 0.0);

			/* Out gamma LUT */
			*argb_line++ = lut_out[lrint(d.b * max_colour)] * 0xff;
			*argb_line++ = lut_out[lrint(d.g * max_colour)] * 0xff;
			*argb_line++ = lut_out[lrint(d.r * max_colour)] * 0xff;
			*argb_line++ = 0xff;
		}

		argb += stride;
	}
}


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
	auto const matrix = conversion.xyz_to_rgb ();

	double fast_matrix[9] = {
		matrix (0, 0), matrix (0, 1), matrix (0, 2),
		matrix (1, 0), matrix (1, 1), matrix (1, 2),
		matrix (2, 0), matrix (2, 1), matrix (2, 2)
	};

	int const height = xyz_image->size().height;
	int const width = xyz_image->size().width;

	for (int y = 0; y < height; ++y) {
		auto rgb_line = reinterpret_cast<uint16_t*> (rgb + y * stride);
		for (int x = 0; x < width; ++x) {

			int cx = *xyz_x++;
			int cy = *xyz_y++;
			int cz = *xyz_z++;

			if (cx < 0 || cx > 4095) {
				if (note) {
					note.get()(NoteType::NOTE, String::compose("XYZ value %1 out of range", cx));
				}
				cx = max (min (cx, 4095), 0);
			}

			if (cy < 0 || cy > 4095) {
				if (note) {
					note.get()(NoteType::NOTE, String::compose("XYZ value %1 out of range", cy));
				}
				cy = max (min (cy, 4095), 0);
			}

			if (cz < 0 || cz > 4095) {
				if (note) {
					note.get()(NoteType::NOTE, String::compose("XYZ value %1 out of range", cz));
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
			d.r = ((s.x * fast_matrix[0]) + (s.y * fast_matrix[1]) + (s.z * fast_matrix[2]));
			d.g = ((s.x * fast_matrix[3]) + (s.y * fast_matrix[4]) + (s.z * fast_matrix[5]));
			d.b = ((s.x * fast_matrix[6]) + (s.y * fast_matrix[7]) + (s.z * fast_matrix[8]));

			d.r = min (d.r, 1.0);
			d.r = max (d.r, 0.0);

			d.g = min (d.g, 1.0);
			d.g = max (d.g, 0.0);

			d.b = min (d.b, 1.0);
			d.b = max (d.b, 0.0);

			*rgb_line++ = lrint(lut_out[lrint(d.r * 65535)] * 65535);
			*rgb_line++ = lrint(lut_out[lrint(d.g * 65535)] * 65535);
			*rgb_line++ = lrint(lut_out[lrint(d.b * 65535)] * 65535);
		}
	}
}

void
dcp::combined_rgb_to_xyz (ColourConversion const & conversion, double* matrix)
{
	auto const rgb_to_xyz = conversion.rgb_to_xyz ();
	auto const bradford = conversion.bradford ();

	matrix[0] = (bradford (0, 0) * rgb_to_xyz (0, 0) + bradford (0, 1) * rgb_to_xyz (1, 0) + bradford (0, 2) * rgb_to_xyz (2, 0))
		* DCI_COEFFICIENT * 65535;
	matrix[1] = (bradford (0, 0) * rgb_to_xyz (0, 1) + bradford (0, 1) * rgb_to_xyz (1, 1) + bradford (0, 2) * rgb_to_xyz (2, 1))
		* DCI_COEFFICIENT * 65535;
	matrix[2] = (bradford (0, 0) * rgb_to_xyz (0, 2) + bradford (0, 1) * rgb_to_xyz (1, 2) + bradford (0, 2) * rgb_to_xyz (2, 2))
		* DCI_COEFFICIENT * 65535;
	matrix[3] = (bradford (1, 0) * rgb_to_xyz (0, 0) + bradford (1, 1) * rgb_to_xyz (1, 0) + bradford (1, 2) * rgb_to_xyz (2, 0))
		* DCI_COEFFICIENT * 65535;
	matrix[4] = (bradford (1, 0) * rgb_to_xyz (0, 1) + bradford (1, 1) * rgb_to_xyz (1, 1) + bradford (1, 2) * rgb_to_xyz (2, 1))
		* DCI_COEFFICIENT * 65535;
	matrix[5] = (bradford (1, 0) * rgb_to_xyz (0, 2) + bradford (1, 1) * rgb_to_xyz (1, 2) + bradford (1, 2) * rgb_to_xyz (2, 2))
		* DCI_COEFFICIENT * 65535;
	matrix[6] = (bradford (2, 0) * rgb_to_xyz (0, 0) + bradford (2, 1) * rgb_to_xyz (1, 0) + bradford (2, 2) * rgb_to_xyz (2, 0))
		* DCI_COEFFICIENT * 65535;
	matrix[7] = (bradford (2, 0) * rgb_to_xyz (0, 1) + bradford (2, 1) * rgb_to_xyz (1, 1) + bradford (2, 2) * rgb_to_xyz (2, 1))
		* DCI_COEFFICIENT * 65535;
	matrix[8] = (bradford (2, 0) * rgb_to_xyz (0, 2) + bradford (2, 1) * rgb_to_xyz (1, 2) + bradford (2, 2) * rgb_to_xyz (2, 2))
		* DCI_COEFFICIENT * 65535;
}


shared_ptr<dcp::OpenJPEGImage>
dcp::rgb_to_xyz (
	uint8_t const * rgb,
	dcp::Size size,
	int stride,
	ColourConversion const & conversion,
	optional<NoteHandler> note
	)
{
	auto xyz = make_shared<OpenJPEGImage>(size);

	struct {
		double r, g, b;
	} s;

	struct {
		double x, y, z;
	} d;

	auto const * lut_in = conversion.in()->lut (12, false);
	auto const * lut_out = conversion.out()->lut (16, true);

	/* This is is the product of the RGB to XYZ matrix, the Bradford transform and the DCI companding */
	double fast_matrix[9];
	combined_rgb_to_xyz (conversion, fast_matrix);

	int clamped = 0;
	int* xyz_x = xyz->data (0);
	int* xyz_y = xyz->data (1);
	int* xyz_z = xyz->data (2);
	for (int y = 0; y < size.height; ++y) {
		auto p = reinterpret_cast<uint16_t const *> (rgb + y * stride);
		for (int x = 0; x < size.width; ++x) {

			/* In gamma LUT (converting 16-bit to 12-bit) */
			s.r = lut_in[*p++ >> 4];
			s.g = lut_in[*p++ >> 4];
			s.b = lut_in[*p++ >> 4];

			/* RGB to XYZ, Bradford transform and DCI companding */
			d.x = s.r * fast_matrix[0] + s.g * fast_matrix[1] + s.b * fast_matrix[2];
			d.y = s.r * fast_matrix[3] + s.g * fast_matrix[4] + s.b * fast_matrix[5];
			d.z = s.r * fast_matrix[6] + s.g * fast_matrix[7] + s.b * fast_matrix[8];

			/* Clamp */

			if (d.x < 0 || d.y < 0 || d.z < 0 || d.x > 65535 || d.y > 65535 || d.z > 65535) {
				++clamped;
			}

			d.x = max (0.0, d.x);
			d.y = max (0.0, d.y);
			d.z = max (0.0, d.z);
			d.x = min (65535.0, d.x);
			d.y = min (65535.0, d.y);
			d.z = min (65535.0, d.z);

			/* Out gamma LUT */
			*xyz_x++ = lrint (lut_out[lrint(d.x)] * 4095);
			*xyz_y++ = lrint (lut_out[lrint(d.y)] * 4095);
			*xyz_z++ = lrint (lut_out[lrint(d.z)] * 4095);
		}
	}

	if (clamped && note) {
		note.get()(NoteType::NOTE, String::compose("%1 XYZ value(s) clamped", clamped));
	}

	return xyz;
}
