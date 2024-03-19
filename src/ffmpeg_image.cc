/*
    Copyright (C) 2024 Carl Hetherington <cth@carlh.net>

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


#include "ffmpeg_image.h"
#include "types.h"
extern "C" {
#include <libavutil/pixfmt.h>
}


using namespace dcp;


FFmpegImage::FFmpegImage(int64_t pts)
{
	auto const width = size().width;
	auto const height = size().height;

	_frame = av_frame_alloc();
	if (!_frame) {
		throw std::bad_alloc();
	}

	_frame->buf[0] = av_buffer_alloc(width * height);
	_frame->buf[1] = av_buffer_alloc(width * height / 4);
	_frame->buf[2] = av_buffer_alloc(width * height / 4);

	_frame->linesize[0] = width;
	_frame->linesize[1] = width / 2;
	_frame->linesize[2] = width / 2;

	for (auto i = 0; i < 3; ++i) {
		_frame->data[i] = _frame->buf[i]->data;
	}

	_frame->width = width;
	_frame->height = height;
	_frame->format = AV_PIX_FMT_YUV420P;
	_frame->pts = pts;
}


void
FFmpegImage::set_pts(int64_t pts)
{
	_frame->pts = pts;
}


uint8_t*
FFmpegImage::y()
{
	return _frame->data[0];
}


int
FFmpegImage::y_stride() const
{
	return _frame->linesize[0];
}


uint8_t*
FFmpegImage::u()
{
	return _frame->data[1];
}


int
FFmpegImage::u_stride() const
{
	return _frame->linesize[1];
}


uint8_t*
FFmpegImage::v()
{
	return _frame->data[2];
}


int
FFmpegImage::v_stride() const
{
	return _frame->linesize[2];
}


