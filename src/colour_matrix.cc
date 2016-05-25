/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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

*/

#include "colour_matrix.h"

/* sRGB colour matrix for XYZ -> RGB.  This is the same as the one used by the Fraunhofer
   EasyDCP player, I think.
*/

double const dcp::colour_matrix::xyz_to_rgb[3][3] = {
	{  3.24096989631653,   -1.5373831987381,  -0.498610764741898 },
	{ -0.96924364566803,    1.87596750259399,  0.0415550582110882 },
	{  0.0556300804018974, -0.203976958990097, 1.05697154998779 }
};

double const dcp::colour_matrix::rgb_to_xyz[3][3] = {
	{0.4124564, 0.3575761, 0.1804375},
	{0.2126729, 0.7151522, 0.0721750},
	{0.0193339, 0.1191920, 0.9503041}
};
