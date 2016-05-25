/*
    Copyright (C) 2015 Carl Hetherington <cth@carlh.net>

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

#include "atmos_asset.h"
#include "test.h"
#include <boost/test/unit_test.hpp>
#include <iostream>

BOOST_AUTO_TEST_CASE (atmos_test)
{
	dcp::AtmosAsset a (private_test / "20160218_NameOfFilm_FTR_OV_EN_A_dcs_r01.mxf");
	BOOST_CHECK_EQUAL (a.first_frame(), 192);
	BOOST_CHECK_EQUAL (a.max_channel_count(), 10);
	BOOST_CHECK_EQUAL (a.max_object_count(), 118);
}
