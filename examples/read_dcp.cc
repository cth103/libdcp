/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

/* If you are using an installed libdcp, these #includes would need to be changed to
#include <libdcp/dcp.h>
#include <libdcp/cpl.h>
#include <libdcp/mono_picture_asset.h>
... etc. ...
*/

#include "dcp.h"

/** @file examples/read_dcp.cc
 *  @brief Shows how to read a DCP.
 */

int
main ()
{
	/* Create a DCP, specifying where our existing data is */
	dcp::DCP dcp ("/home/carl/diagonal.com/APPASSIONATA_TLR_F_UK-DEFR_CH_51_2K_LOK_20121115_DGL_OV");
	/* Read the DCP to find out about it */
	dcp.read ();

	if (dcp.encrypted ()) {
		std::cout << "DCP is encrypted.\n";
	} else {
		std::cout << "DCP is not encrypted.\n";
	}

	std::cout << "DCP has " << dcp.cpls().size() << " CPLs.\n";
	std::cout << "DCP has " << dcp.assets().size() << " assets.\n";

	return 0;
}
