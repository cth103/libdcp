/*
    Copyright (C) 2023 Carl Hetherington <cth@carlh.net>

    This file is part of libdcp.

    libdcp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libdcp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdcp.  If not, see <http://www.gnu.org/licenses/>.

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/


#ifndef LIBDCP_V_ALIGN_H
#define LIBDCP_V_ALIGN_H


#include <string>


namespace dcp {


enum class VAlign
{
	/** vertical position is distance:
	 *    from top of screen to top of subtitle (for SMPTE 428-7:{2007,2010} or
	 *    from top of screen to subtitle baseline (for Interop or SMPTE 428-7:2014)
	 */
	TOP,
	/** vertical position is distance:
	 *    from centre of screen to centre of subtitle (for SMPTE 428-7:{2007,2010}) or
	 *    from centre of screen to subtitle baseline (for Interop or SMPTE 428-7:2014)
	 */
	CENTER,
	/** vertical position is distance:
	 *    from bottom of screen to bottom of subtitle (for SMPTE 428-7:{2007,2010}) or
	 *    from bottom of screen to subtitle baseline (for Interop or SMPTE 428-7:2014)
	 */
	BOTTOM
};


extern std::string valign_to_string(VAlign a);
extern VAlign string_to_valign(std::string s);


}


#endif

