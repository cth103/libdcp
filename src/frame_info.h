/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


#ifndef LIBDCP_FRAME_INFO_H
#define LIBDCP_FRAME_INFO_H


#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <asdcp/AS_DCP.h>
LIBDCP_ENABLE_WARNINGS
#include <stdint.h>
#include <string>


namespace dcp {


/** @class FrameInfo
 *  @brief Information about a single frame (either a monoscopic frame or a left *or* right eye stereoscopic frame)
 */
struct FrameInfo
{
	FrameInfo () = default;

	FrameInfo(uint64_t o, uint64_t s, std::string h)
		: offset(o)
		, size(s)
		, hash(h)
	{}

	uint64_t offset = 0;
	uint64_t size = 0;
	std::string hash;
};


struct J2KFrameInfo : public FrameInfo
{
	J2KFrameInfo() = default;

	J2KFrameInfo(uint64_t offset_, uint64_t size_, std::string hash_)
		: FrameInfo(offset_, size_, hash_)
	{}
};


struct MPEG2FrameInfo : public FrameInfo
{
	MPEG2FrameInfo() = default;

	MPEG2FrameInfo(
		uint64_t offset_,
		uint64_t size_,
		std::string hash_,
		ASDCP::MPEG2::FrameType_t type_,
		bool gop_start_,
		bool closed_gop_,
		uint8_t temporal_offset_
		)
		: FrameInfo(offset_, size_, hash_)
		, type(type_)
		, gop_start(gop_start_)
		, closed_gop(closed_gop_)
		, temporal_offset(temporal_offset_)
	{}

	ASDCP::MPEG2::FrameType_t type;
	bool gop_start;
	bool closed_gop;
	uint8_t temporal_offset;
};


}


#endif
