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

#include "colour_matrix.h"

/* sRGB color matrix for XYZ -> RGB.  This is the same as the one used by the Fraunhofer
   EasyDCP player, I think.
*/

double const libdcp::colour_matrix::xyz_to_rgb[3][3] = {
	{  3.24096989631653,   -1.5373831987381,  -0.498610764741898 },
	{ -0.96924364566803,    1.87596750259399,  0.0415550582110882 },
	{  0.0556300804018974, -0.203976958990097, 1.05697154998779 }
};

double const libdcp::colour_matrix::srgb_to_xyz[3][3] = {
	{0.4124564, 0.3575761, 0.1804375},
	{0.2126729, 0.7151522, 0.0721750},
	{0.0193339, 0.1191920, 0.9503041}
};

double const libdcp::colour_matrix::rec709_to_xyz[3][3] = {
	{ 0.412390799265959,  0.357584339383878, 0.180480788401834 },
	{ 0.21263900587151,   0.715168678767756, 0.0721923153607337 },
        { 0.0193308187155918, 0.119194779794626, 0.950532152249661 }
};
