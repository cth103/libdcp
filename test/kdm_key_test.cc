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
#include "kdm.h"

BOOST_AUTO_TEST_CASE (kdm_key_test)
{
	uint8_t foo[138];
	memset (foo, 0, 138);
	libdcp::KDMKey kkey (foo, 138);

	uint8_t* raw = new uint8_t[16];
	uint8_t* p = raw;
	kkey.put_uuid (&p, "5d51e8a1-b2a5-4da6-9b66-4615c3609440");
	BOOST_CHECK_EQUAL (raw[0], 0x5d);
	BOOST_CHECK_EQUAL (raw[1], 0x51);
	BOOST_CHECK_EQUAL (raw[2], 0xe8);
	BOOST_CHECK_EQUAL (raw[3], 0xa1);
	BOOST_CHECK_EQUAL (raw[4], 0xb2);
	BOOST_CHECK_EQUAL (raw[5], 0xa5);
	BOOST_CHECK_EQUAL (raw[6], 0x4d);
	BOOST_CHECK_EQUAL (raw[7], 0xa6);
	BOOST_CHECK_EQUAL (raw[8], 0x9b);
	BOOST_CHECK_EQUAL (raw[9], 0x66);
	BOOST_CHECK_EQUAL (raw[10], 0x46);
	BOOST_CHECK_EQUAL (raw[11], 0x15);
	BOOST_CHECK_EQUAL (raw[12], 0xc3);
	BOOST_CHECK_EQUAL (raw[13], 0x60);
	BOOST_CHECK_EQUAL (raw[14], 0x94);
	BOOST_CHECK_EQUAL (raw[15], 0x40);
	delete[] raw;
}
