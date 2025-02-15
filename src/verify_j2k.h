/*
    Copyright (C) 2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/verify_j2k.h
 *  @brief Verification that JPEG2000 files meet requirements
 */


#ifndef LIBDCP_VERIFY_J2K_H
#define LIBDCP_VERIFY_J2K_H


#include "verify.h"
#include <vector>


namespace dcp {


class Data;


/** @param start_index Frame index within the DCP where this frame's reel starts.
 *  @param frame_index Video frame index within the reel, so that notes can say which frame contains the problem.
 *  @param frame_rate Video frame rate (in frames per second) to calculate how big the tile parts
 *  can be.
 */
void verify_j2k(std::shared_ptr<const Data> data, int start_index, int frame_index, int frame_rate, std::vector<VerificationNote>& notes);


}


#endif
