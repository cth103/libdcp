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

#ifndef LIBDCP_METADATA_H
#define LIBDCP_METADATA_H

/** @file  src/metadata.h
 *  @brief XMLMetadata and MXFMetadata classes.
 */

#include <string>

class utc_offset_to_string_test;

namespace ASDCP {
	struct WriterInfo;
}

namespace dcp
{

/** @class MXFMetadata
 *  @brief Metadata that is written to a MXF file's header
 */
class MXFMetadata
{
public:
	MXFMetadata ();

	void read (ASDCP::WriterInfo const & info);

	std::string company_name;
	std::string product_name;
	std::string product_version;
};

/** @class XMLMetadata
 *  @brief Common metadata that is written to a few different XML files
 */
class XMLMetadata
{
public:
	XMLMetadata ();

	void set_issue_date_now ();

	std::string issuer;
	std::string creator;
	std::string issue_date;
};

}

#endif
