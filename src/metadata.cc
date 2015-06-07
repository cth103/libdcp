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

/** @file  src/metadata.cc
 *  @brief XMLMetadata and MXFMetadata classes.
 */

#include "metadata.h"
#include "util.h"
#include "local_time.h"
#include "AS_DCP.h"
#include <sstream>
#include <iomanip>
#include <time.h>

using namespace std;
using namespace dcp;

MXFMetadata::MXFMetadata ()
	: company_name ("libdcp")
	, product_name ("libdcp")
	, product_version (LIBDCP_VERSION)
{

}

void
MXFMetadata::read (ASDCP::WriterInfo const & info)
{
	company_name = info.CompanyName;
	product_name = info.ProductName;
	product_version = info.ProductVersion;
}

XMLMetadata::XMLMetadata ()
	: issuer ("libdcp" LIBDCP_VERSION)
	, creator ("libdcp" LIBDCP_VERSION)
{
	set_issue_date_now ();
}

/** Set the issue date to the current local time */
void
XMLMetadata::set_issue_date_now ()
{
	issue_date = LocalTime().as_string ();
}
