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

/** @file  src/metadata.cc
 *  @brief Metadata for writing to the DCP.
 */

#include <time.h>
#include "metadata.h"

using namespace std;
using namespace libdcp;

Metadata* Metadata::_instance = 0;

/** Construct a Metadata object with some default values */
Metadata::Metadata ()
	: company_name ("libdcp")
	, product_name ("libdcp")
	, product_version (LIBDCP_VERSION)
	, issuer ("libdcp" LIBDCP_VERSION)
	, creator ("libdcp" LIBDCP_VERSION)
{
	char buffer[64];
	time_t now;
	time (&now);
	struct tm* tm = localtime (&now);
	strftime (buffer, 64, "%Y-%m-%dT%I:%M:%S+00:00", tm);
	issue_date = string (buffer);
}

/** @return Singleton Metadata instance */
Metadata *
Metadata::instance ()
{
	if (_instance == 0) {
		_instance = new Metadata;
	}

	return _instance;
}
		
