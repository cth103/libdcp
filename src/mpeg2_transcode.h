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


#ifndef LIBDCP_MPEG2_TRANSCODE_H
#define LIBDCP_MPEG2_TRANSCODE_H


#include "ffmpeg_image.h"
#include <memory>


struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;


namespace dcp {


class MonoMPEG2PictureFrame;


class MPEG2Codec
{
public:
	MPEG2Codec() = default;
	virtual ~MPEG2Codec();

	MPEG2Codec(MPEG2Codec const&) = delete;
	MPEG2Codec& operator=(MPEG2Codec const&) = delete;

protected:
	AVCodec const* _codec;
	AVCodecContext* _context;
};


class MPEG2Decompressor : public MPEG2Codec
{
public:
	MPEG2Decompressor();
	~MPEG2Decompressor();

	std::vector<FFmpegImage> decompress_frame(std::shared_ptr<const MonoMPEG2PictureFrame> frame);
	std::vector<FFmpegImage> flush();

private:
	std::vector<FFmpegImage> decompress_packet(AVPacket* packet);

	AVFrame* _decompressed_frame;
};


class MPEG2Compressor : public MPEG2Codec
{
public:
	MPEG2Compressor(dcp::Size size, int video_frame_rate, int64_t bit_rate);

	MPEG2Compressor(MPEG2Compressor const&) = delete;
	MPEG2Compressor& operator=(MPEG2Compressor const&) = delete;

	/** Frame data with frame index within the asset */
	typedef std::pair<std::shared_ptr<MonoMPEG2PictureFrame>, int64_t> IndexedFrame;

	boost::optional<IndexedFrame> compress_frame(FFmpegImage const& image);
	boost::optional<IndexedFrame> flush();

private:
	boost::optional<IndexedFrame> send_and_receive(AVFrame const* frame);
};


}


#endif
