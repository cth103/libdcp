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


/** @file  src/j2k_transcode.cc
 *  @brief Methods to encode and decode JPEG2000
 */


#include "array_data.h"
#include "j2k_transcode.h"
#include "exceptions.h"
#include "openjpeg_image.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include <openjpeg.h>
#include <cmath>
#include <iostream>


using std::min;
using std::pow;
using std::string;
using std::shared_ptr;
using boost::shared_array;
using namespace dcp;


shared_ptr<dcp::OpenJPEGImage>
dcp::decompress_j2k (Data const& data, int reduce)
{
	return dcp::decompress_j2k (data.data(), data.size(), reduce);
}


shared_ptr<dcp::OpenJPEGImage>
dcp::decompress_j2k (shared_ptr<const Data> data, int reduce)
{
	return dcp::decompress_j2k (data->data(), data->size(), reduce);
}


class ReadBuffer
{
public:
	ReadBuffer (uint8_t const * data, int64_t size)
		: _data (data)
		, _size (size)
		, _offset (0)
	{}

	OPJ_SIZE_T read (void* buffer, OPJ_SIZE_T nb_bytes)
	{
		int64_t N = min (nb_bytes, _size - _offset);
		memcpy (buffer, _data + _offset, N);
		_offset += N;
		return N;
	}

private:
	uint8_t const * _data;
	OPJ_SIZE_T _size;
	OPJ_SIZE_T _offset;
};


static OPJ_SIZE_T
read_function (void* buffer, OPJ_SIZE_T nb_bytes, void* data)
{
	return reinterpret_cast<ReadBuffer*>(data)->read (buffer, nb_bytes);
}


static void
read_free_function (void* data)
{
	delete reinterpret_cast<ReadBuffer*>(data);
}


static void
decompress_error_callback (char const * msg, void *)
{
	throw J2KDecompressionError (msg);
}


static void
compress_error_callback (char const * msg, void *)
{
	throw MiscError (msg);
}


shared_ptr<dcp::OpenJPEGImage>
dcp::decompress_j2k (uint8_t const * data, int64_t size, int reduce)
{
	DCP_ASSERT (reduce >= 0);

	uint8_t const jp2_magic[] = {
		0x00,
		0x00,
		0x00,
		0x0c,
		'j',
		'P',
		0x20,
		0x20
	};

	auto format = OPJ_CODEC_J2K;
	if (size >= int (sizeof (jp2_magic)) && memcmp (data, jp2_magic, sizeof (jp2_magic)) == 0) {
		format = OPJ_CODEC_JP2;
	}

	auto decoder = opj_create_decompress (format);
	if (!decoder) {
		boost::throw_exception(ReadError("could not create JPEG2000 decompressor"));
	}
	opj_dparameters_t parameters;
	opj_set_default_decoder_parameters (&parameters);
	parameters.cp_reduce = reduce;
	opj_setup_decoder (decoder, &parameters);

	auto stream = opj_stream_default_create (OPJ_TRUE);
	if (!stream) {
		throw MiscError ("could not create JPEG2000 stream");
	}

	opj_set_error_handler(decoder, decompress_error_callback, 00);

	opj_stream_set_read_function (stream, read_function);
	auto buffer = new ReadBuffer (data, size);
	opj_stream_set_user_data (stream, buffer, read_free_function);
	opj_stream_set_user_data_length (stream, size);

	opj_image_t* image = 0;
	opj_read_header (stream, decoder, &image);
	if (opj_decode (decoder, stream, image) == OPJ_FALSE) {
		opj_destroy_codec (decoder);
		opj_stream_destroy (stream);
		if (format == OPJ_CODEC_J2K) {
			boost::throw_exception (ReadError (String::compose ("could not decode JPEG2000 codestream of %1 bytes.", size)));
		} else {
			boost::throw_exception (ReadError (String::compose ("could not decode JP2 file of %1 bytes.", size)));
		}
	}

	opj_destroy_codec (decoder);
	opj_stream_destroy (stream);

	image->x1 = rint (float(image->x1) / pow (2.0f, reduce));
	image->y1 = rint (float(image->y1) / pow (2.0f, reduce));
	return std::make_shared<OpenJPEGImage>(image);
}


class WriteBuffer
{
public:
	OPJ_SIZE_T write (void* buffer, OPJ_SIZE_T nb_bytes)
	{
		auto const new_offset = _offset + nb_bytes;
		if (new_offset > OPJ_SIZE_T(_data.size())) {
			_data.set_size(new_offset);
		}
		memcpy(_data.data() + _offset, buffer, nb_bytes);
		_offset = new_offset;
		return nb_bytes;
	}

	OPJ_BOOL seek (OPJ_SIZE_T nb_bytes)
	{
		_offset = nb_bytes;
		return OPJ_TRUE;
	}

	ArrayData data () const
	{
		return _data;
	}

private:
	ArrayData _data;
	OPJ_SIZE_T _offset = 0;
};


static OPJ_SIZE_T
write_function (void* buffer, OPJ_SIZE_T nb_bytes, void* data)
{
	return reinterpret_cast<WriteBuffer*>(data)->write(buffer, nb_bytes);
}


static void
write_free_function (void* data)
{
	delete reinterpret_cast<WriteBuffer*>(data);
}


static OPJ_BOOL
seek_function (OPJ_OFF_T nb_bytes, void* data)
{
	return reinterpret_cast<WriteBuffer*>(data)->seek(nb_bytes);

}


ArrayData
dcp::compress_j2k (shared_ptr<const OpenJPEGImage> xyz, int bandwidth, int frames_per_second, bool threed, bool fourk, string comment)
{
	/* get a J2K compressor handle */
	auto encoder = opj_create_compress (OPJ_CODEC_J2K);
	if (encoder == nullptr) {
		throw MiscError ("could not create JPEG2000 encoder");
	}

	if (comment.empty()) {
		/* asdcplib complains with "Illegal data size" when reading frames encoded with an empty comment */
		throw MiscError("compress_j2k comment can not be an empty string");
	}

	opj_set_error_handler (encoder, compress_error_callback, 0);

	/* Set encoding parameters to default values */
	opj_cparameters_t parameters;
	opj_set_default_encoder_parameters (&parameters);
	if (fourk) {
		parameters.numresolution = 7;
	}
	parameters.rsiz = fourk ? OPJ_PROFILE_CINEMA_4K : OPJ_PROFILE_CINEMA_2K;
	parameters.cp_comment = strdup (comment.c_str());

	/* set max image */
	parameters.max_cs_size = (bandwidth / 8) / frames_per_second;
	if (threed) {
		/* In 3D we have only half the normal bandwidth per eye */
		parameters.max_cs_size /= 2;
	}
	parameters.max_comp_size = parameters.max_cs_size / 1.25;
	parameters.tcp_numlayers = 1;
	parameters.tcp_mct = 1;
#ifdef LIBDCP_HAVE_NUMGBITS
	parameters.numgbits = fourk ? 2 : 1;
#endif

	/* Setup the encoder parameters using the current image and user parameters */
	opj_setup_encoder (encoder, &parameters, xyz->opj_image());

#ifndef LIBDCP_HAVE_NUMGBITS
	string numgbits = String::compose("GUARD_BITS=%1", fourk ? 2 : 1);
	char const* extra_options[] = { numgbits.c_str(), nullptr };
	opj_encoder_set_extra_options(encoder, extra_options);
#endif

	auto stream = opj_stream_default_create (OPJ_FALSE);
	if (!stream) {
		opj_destroy_codec (encoder);
		free (parameters.cp_comment);
		throw MiscError ("could not create JPEG2000 stream");
	}

	opj_stream_set_write_function (stream, write_function);
	opj_stream_set_seek_function (stream, seek_function);
	WriteBuffer* buffer = new WriteBuffer ();
	opj_stream_set_user_data (stream, buffer, write_free_function);

	if (!opj_start_compress (encoder, xyz->opj_image(), stream)) {
		opj_stream_destroy (stream);
		opj_destroy_codec (encoder);
		free (parameters.cp_comment);
		if ((errno & 0x61500) == 0x61500) {
			/* We've had one of the magic error codes from our patched openjpeg */
			boost::throw_exception (StartCompressionError (errno & 0xff));
		} else {
			boost::throw_exception (StartCompressionError ());
		}
	}

	if (!opj_encode (encoder, stream)) {
		opj_stream_destroy (stream);
		opj_destroy_codec (encoder);
		free (parameters.cp_comment);
		throw MiscError ("JPEG2000 encoding failed");
	}

	if (!opj_end_compress (encoder, stream)) {
		opj_stream_destroy (stream);
		opj_destroy_codec (encoder);
		free (parameters.cp_comment);
		throw MiscError ("could not end JPEG2000 encoding");
	}

	ArrayData enc (buffer->data ());

	opj_stream_destroy (stream);
	opj_destroy_codec (encoder);
	free (parameters.cp_comment);

	return enc;
}

