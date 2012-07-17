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

/** @file  src/metadata.h
 *  @brief Metadata for writing to the DCP.
 */

#include <string>

namespace libdcp
{

/** @brief A class to hold various metadata that will be written
 *  to the DCP.
 *
 *  The values are initialised, and can be modified if desired.
 */
class Metadata
{
public:
	static Metadata* instance ();

	std::string company_name;
	std::string product_name;
	std::string product_version;
	std::string issuer;
	std::string creator;
	std::string issue_date;

private:
	Metadata ();

	/** Singleton instance of Metadata */
	static Metadata* _instance;
};

}
