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

#include <boost/test/unit_test.hpp>
#include "opendcp_lut.h"
#include "opendcp_lut.cc"
#include "srgb_linearised_gamma_lut.h"
#include "rec709_linearised_gamma_lut.h"
#include "gamma_lut.h"

/* Check that some of our LUTs match the ones from OpenDCP that
   DVD-o-matic uses / once used.
*/
BOOST_AUTO_TEST_CASE (lut_test)
{
	libdcp::SRGBLinearisedGammaLUT lut_in_srgb (12, 2.4);
	for (int i = 0; i < 4096; ++i) {
		/* Hmm; 1% isn't exactly great... */
		BOOST_CHECK_CLOSE (opendcp::lut_in[0][i], lut_in_srgb.lut()[i], 1);
	}

	libdcp::Rec709LinearisedGammaLUT lut_in_rec709 (12, 1 / 0.45);
	for (int i = 0; i < 4096; ++i) {
		/* Hmm; 1% isn't exactly great... */
		BOOST_CHECK_CLOSE (opendcp::lut_in[1][i], lut_in_rec709.lut()[i], 1);
	}

	libdcp::GammaLUT lut_out (16, 1 / 2.6);
	for (int i = 0; i < 65536; ++i) {
		BOOST_CHECK_CLOSE (opendcp::lut_out[0][i], lut_out.lut()[i] * 4096, 1);
	}
}
