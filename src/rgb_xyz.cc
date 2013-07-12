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

using std::min;
using std::max;
using boost::shared_ptr;
using namespace libdcp;

/** Convert an openjpeg XYZ image to RGB.
 *  @param xyz_frame Frame in XYZ.
 *  @return RGB image.
 */
shared_ptr<ARGBFrame>
libdcp::xyz_to_rgb (shared_ptr<const XYZFrame> xyz_frame, shared_ptr<const GammaLUT> lut_in, shared_ptr<const GammaLUT> lut_out)
{
	float const dci_coefficient = 48.0 / 52.37;

        /* sRGB color matrix for XYZ -> RGB.  This is the same as the one used by the Fraunhofer
	   EasyDCP player, I think.
	*/

	float const colour_matrix[3][3] = {
		{  3.24096989631653,   -1.5373831987381,  -0.498610764741898 },
		{ -0.96924364566803,    1.87596750259399,  0.0415550582110882 },
		{  0.0556300804018974, -0.203976958990097, 1.05697154998779 }
	};

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
			*argb_line++ = lut_out->lut()[(int) (d.b * max_colour)] * 0xff;
			*argb_line++ = lut_out->lut()[(int) (d.g * max_colour)] * 0xff;
			*argb_line++ = lut_out->lut()[(int) (d.r * max_colour)] * 0xff;
			*argb_line++ = 0xff;
		}
		
		argb += argb_frame->stride ();
	}

	return argb_frame;
}

