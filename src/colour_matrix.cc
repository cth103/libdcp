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

/* From Dennis Couzin via email */
double const libdcp::colour_matrix::rec601_to_xyz[3][3] = {
	{ 0.3935276, 0.3652562, 0.1916771 },
	{ 0.2123800, 0.7010562, 0.0865638 },
	{ 0.0187394, 0.1119333, 0.9583854 }
};

/* These are the same, but kept as two separate variables for backwards compatibility;
   the confusion is fixed in the 1.0 branch.
*/

double const libdcp::colour_matrix::srgb_to_xyz[3][3] = {
	{ 0.4123908, 0.3575843, 0.1804808 },
	{ 0.2126390, 0.7151687, 0.0721923 },
	{ 0.0193308, 0.1191948, 0.9505322 }
};

double const libdcp::colour_matrix::rec709_to_xyz[3][3] = {
	{ 0.4123908, 0.3575843, 0.1804808 },
	{ 0.2126390, 0.7151687, 0.0721923 },
	{ 0.0193308, 0.1191948, 0.9505322 }
};
