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


#ifndef LIBDCP_FFMPEG_IMAGE_H
#define LIBDCP_FFMPEG_IMAGE_H


#include "types.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
extern "C" {
#include <libavutil/frame.h>
}
LIBDCP_ENABLE_WARNINGS
#include <algorithm>
#include <vector>


namespace dcp {


class FFmpegImage
{
public:
	explicit FFmpegImage(int64_t pts);

	explicit FFmpegImage(AVFrame* frame)
		: _frame(frame)
	{}

	FFmpegImage(FFmpegImage const& other) = delete;
	FFmpegImage& operator=(FFmpegImage const& other) = delete;

	FFmpegImage(FFmpegImage&& other) {
		std::swap(_frame, other._frame);
	}

	FFmpegImage& operator=(FFmpegImage&& other) {
		std::swap(_frame, other._frame);
		return *this;
	}

	~FFmpegImage()
	{
		av_frame_free(&_frame);
	}

	AVFrame const * frame() const {
		return _frame;
	}

	uint8_t* y();
	int y_stride() const;

	uint8_t* u();
	int u_stride() const;

	uint8_t* v();
	int v_stride() const;

	Size size() const {
		return { 1920, 1080 };
	}

	void set_pts(int64_t pts);

private:
	AVFrame* _frame = nullptr;
};


}


#endif

