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

#include <sstream>
#include <iomanip>
#include <time.h>
#ifdef LIBDCP_WINDOWS
#include <windows.h>
#endif
#include "metadata.h"

using namespace std;
using namespace libdcp;

MXFMetadata::MXFMetadata ()
	: company_name ("libdcp")
	, product_name ("libdcp")
	, product_version (LIBDCP_VERSION)
{

}


XMLMetadata::XMLMetadata ()
	: issuer ("libdcp" LIBDCP_VERSION)
	, creator ("libdcp" LIBDCP_VERSION)
{
	set_issue_date_now ();
}

void
XMLMetadata::set_issue_date_now ()
{
	char buffer[64];
	time_t now;
	time (&now);
	struct tm* tm = localtime (&now);
#ifdef LIBDCP_POSIX	
	strftime (buffer, 64, "%Y-%m-%dT%I:%M:%S%z", tm);
	issue_date = string (buffer);
#else
	/* No %z for strftime on Windows: it will seemingly be interpreted as %Z and will
	   output some localised string describing the timezone */
	strftime (buffer, 64, "%Y-%m-%dT%I:%M:%S", tm);

	TIME_ZONE_INFORMATION tz;
	GetTimeZoneInformation (&tz);
	issue_date = string (buffer) + bias_to_string (tz.Bias);
#endif
}

string
XMLMetadata::bias_to_string (int b)
{
	bool const negative = (b < 0);
	b = negative ? -b : b;

	int const hours = b / 60;
	int const minutes = b % 60;

	stringstream o;
	if (negative) {
		o << "-";
	} else {
		o << "+";
	}

	o << setw(2) << setfill('0') << hours << setw(2) << setfill('0') << minutes;
	return o.str ();
}
