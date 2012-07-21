/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

/** @file  src/test_mode.cc
 *  @brief A method to enable test mode for libdcp.
 */

#include "KM_prng.h"
#include "test_mode.h"
#include "metadata.h"

/** Calling this will seed the random number generator used to
 *  generate UUIDs with a known value, and set the DCP issue
 *  date to 1st January 2012 at midnight.  This means that
 *  two runs of libdcp with the same inputs will produce
 *  the same output.
 */

void
libdcp::enable_test_mode ()
{
	Kumu::libdcp_test = true;
	Metadata::instance()->issue_date = "2012-01-01T00:00:00+00:00";

	/* Remove version strings */
	Metadata::instance()->issuer = "libdcp-test";
	Metadata::instance()->creator = "libdcp-test";
	Metadata::instance()->product_version = "test";
}
