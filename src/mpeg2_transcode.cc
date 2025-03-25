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


#include "compose.hpp"
#include "exceptions.h"
#include "mono_mpeg2_picture_frame.h"
#include "mpeg2_transcode.h"
#include "scope_guard.h"
extern "C" {
#include <libavcodec/avcodec.h>
}


using std::make_shared;
using std::shared_ptr;
using std::vector;
using boost::optional;
using namespace dcp;


MPEG2Codec::~MPEG2Codec()
{
	avcodec_free_context(&_context);
}



MPEG2Decompressor::MPEG2Decompressor()
{
	_codec = avcodec_find_decoder_by_name("mpeg2video");
	if (!_codec) {
		throw MPEG2CodecError("could not find codec");
	}

	_context = avcodec_alloc_context3(_codec);
	if (!_context) {
		throw MPEG2CodecError("could not allocate codec context");
	}

	int const r = avcodec_open2(_context, _codec, nullptr);
	if (r < 0) {
		avcodec_free_context(&_context);
		throw MPEG2CodecError("could not open codec");
	}

	_decompressed_frame = av_frame_alloc();
	if (!_decompressed_frame) {
		throw std::bad_alloc();
	}
}


MPEG2Decompressor::~MPEG2Decompressor()
{
	av_frame_free(&_decompressed_frame);
}


vector<FFmpegImage>
MPEG2Decompressor::decompress_frame(shared_ptr<const MonoMPEG2PictureFrame> frame)
{
	/* XXX: can we avoid this? */
	auto copy = av_malloc(frame->size() + AV_INPUT_BUFFER_PADDING_SIZE);
	if (!copy) {
		throw std::bad_alloc();
	}
	memcpy(copy, frame->data(), frame->size());

	AVPacket packet;
	av_init_packet(&packet);
	av_packet_from_data(&packet, reinterpret_cast<uint8_t*>(copy), frame->size());

	auto images = decompress_packet(&packet);

	av_packet_unref(&packet);

	return images;
}


vector<FFmpegImage>
MPEG2Decompressor::flush()
{
	return decompress_packet(nullptr);
}


vector<FFmpegImage>
MPEG2Decompressor::decompress_packet(AVPacket* packet)
{
	int const r = avcodec_send_packet(_context, packet);
	if (r < 0) {
		throw MPEG2DecompressionError(String::compose("avcodec_send_packet failed (%1)", r));
	}

	vector<FFmpegImage> images;
	while (true) {
		int const r = avcodec_receive_frame(_context, _decompressed_frame);
		if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) {
			break;
		} else if (r < 0) {
			throw MPEG2DecompressionError("avcodec_receive_frame failed");
		}

		auto clone = av_frame_clone(_decompressed_frame);
		if (!clone) {
			throw std::bad_alloc();
		}

		images.push_back(FFmpegImage(clone));
	}

	return images;
}


MPEG2Compressor::MPEG2Compressor(dcp::Size size, int video_frame_rate, int64_t bit_rate)
{
	_codec = avcodec_find_encoder_by_name("mpeg2video");
	if (!_codec) {
		throw MPEG2CodecError("could not find codec");
	}

	_context = avcodec_alloc_context3(_codec);
	if (!_context) {
		throw MPEG2CodecError("could not allocate codec context");
	}

	_context->width = size.width;
	_context->height = size.height;
	_context->time_base = AVRational{1, video_frame_rate};
	_context->pix_fmt = AV_PIX_FMT_YUV420P;
	_context->bit_rate = bit_rate;
	_context->rc_max_rate = bit_rate;
	_context->rc_min_rate = bit_rate;

	int const r = avcodec_open2(_context, _codec, nullptr);
	if (r < 0) {
		avcodec_free_context(&_context);
		throw MPEG2CodecError("could not open codec");
	}
}


optional<MPEG2Compressor::IndexedFrame>
MPEG2Compressor::send_and_receive(AVFrame const* frame)
{
	int r = avcodec_send_frame(_context, frame);
	if (r < 0) {
		throw MPEG2CompressionError(String::compose("avcodec_send_frame failed (%1", r));
	}

	auto packet = av_packet_alloc();
	if (!packet) {
		throw MPEG2CompressionError("could not allocate packet");
	}

	r = avcodec_receive_packet(_context, packet);
	if (r < 0 && r != AVERROR(EAGAIN)) {
		throw MPEG2CompressionError(String::compose("avcodec_receive_packet failed (%1)", r));
	}

	ScopeGuard sg = [&packet]() {
		av_packet_free(&packet);
	};

	if (packet->size == 0) {
		return {};
	}

	DCP_ASSERT(_context->time_base.num == 1);
	return IndexedFrame{make_shared<MonoMPEG2PictureFrame>(packet->data, packet->size), std::round(static_cast<double>(packet->pts) / _context->time_base.den)};
}


optional<MPEG2Compressor::IndexedFrame>
MPEG2Compressor::compress_frame(FFmpegImage const& image)
{
	return send_and_receive(image.frame());
}


optional<MPEG2Compressor::IndexedFrame>
MPEG2Compressor::flush()
{
	return send_and_receive(nullptr);
}
